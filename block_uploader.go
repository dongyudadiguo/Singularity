package main

import (
	"crypto/sha256"
	"encoding/binary"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"time"
)

const uploadMaxBody = 256 << 20

type uploadResult struct {
	Name string `json:"name"`
	Hash string `json:"hash,omitempty"`
	Err  string `json:"error,omitempty"`
}

func main() {
	addr := flag.String("addr", "118.25.42.70:9000", "CVM server address")
	listen := flag.String("listen", "127.0.0.1:8787", "local web uploader address")
	web := flag.Bool("web", false, "start the drag-and-drop web uploader")
	flag.Parse()

	files := flag.Args()
	if len(files) == 0 || *web {
		must(serveWebUploader(*listen, *addr))
		return
	}

	results := uploadFiles(*addr, files)
	failed := false
	for _, r := range results {
		if r.Err != "" {
			failed = true
			fmt.Printf("error\t%s\t%s\n", r.Name, r.Err)
			continue
		}
		fmt.Printf("uploaded\t%s\t%s\n", r.Name, r.Hash)
	}
	if failed {
		os.Exit(1)
	}
}

func uploadFiles(addr string, paths []string) []uploadResult {
	conn, err := net.DialTimeout("tcp", addr, 10*time.Second)
	if err != nil {
		out := make([]uploadResult, len(paths))
		for i, p := range paths {
			out[i] = uploadResult{Name: filepath.Base(p), Err: err.Error()}
		}
		return out
	}
	defer conn.Close()

	out := make([]uploadResult, 0, len(paths))
	for _, path := range paths {
		data, err := os.ReadFile(path)
		if err == nil && len(data) > uploadMaxBody {
			err = fmt.Errorf("file is too large: %d bytes", len(data))
		}
		if err != nil {
			out = append(out, uploadResult{Name: filepath.Base(path), Err: err.Error()})
			continue
		}
		hash, err := uploadBytes(conn, data)
		if err != nil {
			out = append(out, uploadResult{Name: filepath.Base(path), Err: err.Error()})
			continue
		}
		out = append(out, uploadResult{Name: filepath.Base(path), Hash: hash})
	}
	return out
}

func serveWebUploader(listen, addr string) error {
	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/" {
			http.NotFound(w, r)
			return
		}
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		_, _ = io.WriteString(w, uploaderHTML)
	})
	mux.HandleFunc("/upload", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		r.Body = http.MaxBytesReader(w, r.Body, int64(uploadMaxBody)*20)
		if err := r.ParseMultipartForm(32 << 20); err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		files := r.MultipartForm.File["files"]
		results := make([]uploadResult, 0, len(files))

		conn, err := net.DialTimeout("tcp", addr, 10*time.Second)
		if err != nil {
			for _, fh := range files {
				results = append(results, uploadResult{Name: fh.Filename, Err: err.Error()})
			}
			writeJSON(w, results)
			return
		}
		defer conn.Close()

		for _, fh := range files {
			func() {
				f, err := fh.Open()
				if err != nil {
					results = append(results, uploadResult{Name: fh.Filename, Err: err.Error()})
					return
				}
				defer f.Close()

				data, err := io.ReadAll(f)
				if err == nil && len(data) > uploadMaxBody {
					err = fmt.Errorf("file is too large: %d bytes", len(data))
				}
				if err != nil {
					results = append(results, uploadResult{Name: fh.Filename, Err: err.Error()})
					return
				}
				hash, err := uploadBytes(conn, data)
				if err != nil {
					results = append(results, uploadResult{Name: fh.Filename, Err: err.Error()})
					return
				}
				results = append(results, uploadResult{Name: fh.Filename, Hash: hash})
			}()
		}
		writeJSON(w, results)
	})

	fmt.Printf("drop files at http://%s/\n", listen)
	return http.ListenAndServe(listen, mux)
}

func uploadBytes(c net.Conn, body []byte) (string, error) {
	status, out, err := request(c, 2, body)
	if err != nil {
		return "", err
	}
	if status != 0 {
		return "", fmt.Errorf("upload status %d", status)
	}
	sum := sha256.Sum256(body)
	if len(out) != 32 || string(out) != string(sum[:]) {
		return "", errors.New("upload hash mismatch")
	}
	return fmt.Sprintf("%x", out), nil
}

func request(c net.Conn, op byte, body []byte) (byte, []byte, error) {
	var h [5]byte
	h[0] = op
	binary.BigEndian.PutUint32(h[1:5], uint32(len(body)))
	if _, err := c.Write(h[:]); err != nil {
		return 0, nil, err
	}
	if len(body) != 0 {
		if _, err := c.Write(body); err != nil {
			return 0, nil, err
		}
	}
	if _, err := io.ReadFull(c, h[:]); err != nil {
		return 0, nil, err
	}
	n := binary.BigEndian.Uint32(h[1:5])
	out := make([]byte, n)
	if n != 0 {
		if _, err := io.ReadFull(c, out); err != nil {
			return 0, nil, err
		}
	}
	return h[0], out, nil
}

func writeJSON(w http.ResponseWriter, v any) {
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(v)
}

func must(err error) {
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

const uploaderHTML = `<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CVM Block Uploader</title>
<style>
body{margin:0;background:#0f172a;color:#e2e8f0;font:16px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
main{max-width:880px;margin:48px auto;padding:0 20px}
h1{font-size:30px;margin:0 0 8px}.muted{color:#94a3b8;margin:0 0 24px}
#drop{border:2px dashed #38bdf8;border-radius:22px;padding:58px 24px;text-align:center;background:#111c33;transition:.15s}
#drop.drag{background:#082f49;border-color:#7dd3fc;transform:scale(1.01)}
button{margin-top:18px;border:0;border-radius:12px;padding:12px 18px;background:#2563eb;color:white;font-weight:700;cursor:pointer}
pre{white-space:pre-wrap;background:#020617;border:1px solid #334155;border-radius:14px;padding:16px;min-height:120px}
</style>
</head>
<body>
<main>
<h1>CVM Block Uploader</h1>
<p class="muted">把多个块文件拖进下面区域，上传后会返回内容哈希。</p>
<div id="drop">拖拽文件到这里<br><button id="pick" type="button">选择文件</button><input id="files" type="file" multiple hidden></div>
<h2>结果</h2><pre id="out">等待文件...</pre>
</main>
<script>
const drop=document.querySelector('#drop'), input=document.querySelector('#files'), out=document.querySelector('#out');
document.querySelector('#pick').onclick=()=>input.click();
input.onchange=()=>upload(input.files);
for(const ev of ['dragenter','dragover']) drop.addEventListener(ev,e=>{e.preventDefault();drop.classList.add('drag')});
for(const ev of ['dragleave','drop']) drop.addEventListener(ev,e=>{e.preventDefault();drop.classList.remove('drag')});
drop.addEventListener('drop',e=>upload(e.dataTransfer.files));
async function upload(files){
  if(!files.length)return;
  const form=new FormData();
  for(const f of files)form.append('files',f,f.name);
  out.textContent='上传中...';
  const resp=await fetch('/upload',{method:'POST',body:form});
  const text=await resp.text();
  if(!resp.ok){out.textContent=text;return;}
  const rows=JSON.parse(text);
  out.textContent=rows.map(r=>r.error?'ERROR\t'+r.name+'\t'+r.error:'OK\t'+r.name+'\t'+r.hash).join('\n');
}
</script>
</body>
</html>`
