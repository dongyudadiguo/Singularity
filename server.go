package main

import (
"bytes"
"crypto/sha256"
"encoding/binary"
"encoding/gob"
"encoding/hex"
"fmt"
"io"
"log"
"net/http"
"os"
"path/filepath"
"sort"
"sync"
)

const (
H         = 32
maxUpload = 256 << 20
maxToken  = 1 << 20
)

type Hash [H]byte
type Identity [H]byte

type State struct {
Edges  map[Hash]map[Hash]bool
Votes  map[Identity]map[Hash]Hash
UserKV map[Identity]map[Hash]Hash
}

var (
mu sync.RWMutex

edges  map[Hash]map[Hash]bool
votes  map[Identity]map[Hash]Hash
userKV map[Identity]map[Hash]Hash

dataDir   = "data"
filesDir  = filepath.Join(dataDir, "files")
stateFile = filepath.Join(dataDir, "state.gob")
)

func hashFrom(b []byte) Hash {
var h Hash
copy(h[:], b)
return h
}

func identityFrom(b []byte) Identity {
var id Identity
copy(id[:], b)
return id
}

func fail(w http.ResponseWriter, code int, msg string) {
http.Error(w, msg, code)
}

func bin(w http.ResponseWriter, code int, b []byte) {
w.Header().Set("Content-Type", "application/octet-stream")
w.WriteHeader(code)
if len(b) != 0 {
_, _ = w.Write(b)
}
}

func empty(w http.ResponseWriter) {
bin(w, http.StatusNoContent, nil)
}

func only(method string, h http.HandlerFunc) http.HandlerFunc {
return func(w http.ResponseWriter, r *http.Request) {
if r.Method != method {
fail(w, http.StatusMethodNotAllowed, method+" only")
return
}
h(w, r)
}
}

func readExact(w http.ResponseWriter, r *http.Request, n int) ([]byte, bool) {
b, err := io.ReadAll(io.LimitReader(r.Body, int64(n+1)))
if err != nil {
fail(w, http.StatusBadRequest, "bad body")
return nil, false
}
if len(b) != n {
fail(w, http.StatusBadRequest, fmt.Sprintf("want %d bytes", n))
return nil, false
}
return b, true
}

func readMax(w http.ResponseWriter, r *http.Request, max int64) ([]byte, bool) {
b, err := io.ReadAll(io.LimitReader(r.Body, max+1))
if err != nil {
fail(w, http.StatusBadRequest, "bad body")
return nil, false
}
if int64(len(b)) > max {
fail(w, http.StatusRequestEntityTooLarge, "body too large")
return nil, false
}
return b, true
}

func initState() {
edges = map[Hash]map[Hash]bool{}
votes = map[Identity]map[Hash]Hash{}
userKV = map[Identity]map[Hash]Hash{}
}

func loadState() {
initState()

f, err := os.Open(stateFile)
if err != nil {
return
}
defer f.Close()

var s State
if gob.NewDecoder(f).Decode(&s) != nil {
return
}

if s.Edges != nil {
edges = s.Edges
}
if s.Votes != nil {
votes = s.Votes
}
if s.UserKV != nil {
userKV = s.UserKV
}
}

func saveLocked() error {
tmp := stateFile + ".tmp"

f, err := os.Create(tmp)
if err != nil {
return err
}

err = gob.NewEncoder(f).Encode(State{
Edges:  edges,
Votes:  votes,
UserKV: userKV,
})

if cerr := f.Close(); err == nil {
err = cerr
}

if err != nil {
_ = os.Remove(tmp)
return err
}

return os.Rename(tmp, stateFile)
}

func addEdgeLocked(parent, child Hash) {
m := edges[parent]
if m == nil {
m = map[Hash]bool{}
edges[parent] = m
}
m[child] = true
}

func filePath(h Hash) string {
return filepath.Join(filesDir, hex.EncodeToString(h[:]))
}

func apiRegister(w http.ResponseWriter, r *http.Request) {
b, ok := readMax(w, r, maxToken)
if !ok {
return
}

sum := sha256.Sum256(b)
id := Identity(sum)

bin(w, http.StatusOK, id[:])
}

func apiUpload(w http.ResponseWriter, r *http.Request) {
b, ok := readMax(w, r, maxUpload)
if !ok {
return
}

sum := sha256.Sum256(b)
h := Hash(sum)
p := filePath(h)

if _, err := os.Stat(p); os.IsNotExist(err) {
tmp := p + ".tmp"

if err := os.WriteFile(tmp, b, 0644); err != nil {
fail(w, http.StatusInternalServerError, err.Error())
return
}

if err := os.Rename(tmp, p); err != nil {
_ = os.Remove(tmp)
fail(w, http.StatusInternalServerError, err.Error())
return
}
}

bin(w, http.StatusOK, h[:])
}

func apiFile(w http.ResponseWriter, r *http.Request) {
b, ok := readExact(w, r, 32)
if !ok {
return
}

h := hashFrom(b)

file, err := os.ReadFile(filePath(h))
if err != nil {
fail(w, http.StatusNotFound, "file not found")
return
}

bin(w, http.StatusOK, file)
}

func apiEdge(w http.ResponseWriter, r *http.Request) {
b, ok := readExact(w, r, 64)
if !ok {
return
}

parent := hashFrom(b[:32])
child := hashFrom(b[32:64])

mu.Lock()
addEdgeLocked(parent, child)
err := saveLocked()
mu.Unlock()

if err != nil {
fail(w, http.StatusInternalServerError, err.Error())
return
}

empty(w)
}

type row struct {
child Hash
score int64
}

func apiChildren(w http.ResponseWriter, r *http.Request) {
b, ok := readExact(w, r, 32)
if !ok {
return
}

parent := hashFrom(b)

mu.RLock()

var rows []row

for child := range edges[parent] {
var score int64

for _, byParent := range votes {
if v, ok := byParent[parent]; ok && v == child {
score++
}
}

rows = append(rows, row{child: child, score: score})
}

mu.RUnlock()

sort.Slice(rows, func(i, j int) bool {
if rows[i].score != rows[j].score {
return rows[i].score > rows[j].score
}
return bytes.Compare(rows[i].child[:], rows[j].child[:]) < 0
})

out := make([]byte, 4+len(rows)*40)
binary.BigEndian.PutUint32(out[:4], uint32(len(rows)))

off := 4
for _, r := range rows {
copy(out[off:off+32], r.child[:])
off += 32

binary.BigEndian.PutUint64(out[off:off+8], uint64(r.score))
off += 8
}

bin(w, http.StatusOK, out)
}

func apiVote(w http.ResponseWriter, r *http.Request) {
b, ok := readExact(w, r, 96)
if !ok {
return
}

user := identityFrom(b[:32])
parent := hashFrom(b[32:64])
child := hashFrom(b[64:96])

mu.Lock()

addEdgeLocked(parent, child)

m := votes[user]
if m == nil {
m = map[Hash]Hash{}
votes[user] = m
}

m[parent] = child

err := saveLocked()

mu.Unlock()

if err != nil {
fail(w, http.StatusInternalServerError, err.Error())
return
}

empty(w)
}

func apiUserSet(w http.ResponseWriter, r *http.Request) {
b, ok := readExact(w, r, 96)
if !ok {
return
}

user := identityFrom(b[:32])
key := hashFrom(b[32:64])
val := hashFrom(b[64:96])

mu.Lock()

m := userKV[user]
if m == nil {
m = map[Hash]Hash{}
userKV[user] = m
}

m[key] = val

err := saveLocked()

mu.Unlock()

if err != nil {
fail(w, http.StatusInternalServerError, err.Error())
return
}

empty(w)
}

func apiUserGet(w http.ResponseWriter, r *http.Request) {
b, ok := readExact(w, r, 64)
if !ok {
return
}

user := identityFrom(b[:32])
key := hashFrom(b[32:64])

var val Hash

mu.RLock()
if m := userKV[user]; m != nil {
val = m[key]
}
mu.RUnlock()

bin(w, http.StatusOK, val[:])
}

func main() {
if err := os.MkdirAll(filesDir, 0755); err != nil {
log.Fatal(err)
}

loadState()

http.HandleFunc("/api/register", only("POST", apiRegister))
http.HandleFunc("/api/upload", only("POST", apiUpload))
http.HandleFunc("/api/file", only("POST", apiFile))
http.HandleFunc("/api/edge", only("POST", apiEdge))
http.HandleFunc("/api/children", only("POST", apiChildren))
http.HandleFunc("/api/vote", only("POST", apiVote))
http.HandleFunc("/api/user/set", only("POST", apiUserSet))
http.HandleFunc("/api/user/get", only("POST", apiUserGet))

log.Println("CVM binary server listening on :9000")
log.Fatal(http.ListenAndServe(":9000", nil))
}