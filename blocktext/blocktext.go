package blocktext

import (
	"bufio"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"os"
	"strconv"
	"strings"
)

const HashSize = 32

type Hash [HashSize]byte

type TokenMap map[string]Hash

type SourceBuilder struct {
	b strings.Builder
}

func (s *SourceBuilder) Token(name string) {
	s.b.WriteString(name)
	s.b.WriteByte('\n')
}

func (s *SourceBuilder) Bytes(name string, payload []byte) {
	if len(payload) == 0 {
		s.Token(name)
		return
	}
	fmt.Fprintf(&s.b, "%s hex %x\n", name, payload)
}

func (s *SourceBuilder) Hash(name string, h [HashSize]byte) {
	fmt.Fprintf(&s.b, "%s hash %x\n", name, h)
}

func (s *SourceBuilder) U64(name string, v uint64) {
	fmt.Fprintf(&s.b, "%s u64 %d\n", name, v)
}

func (s *SourceBuilder) String(name, payload string) {
	fmt.Fprintf(&s.b, "%s str %s\n", name, strconv.Quote(payload))
}

func (s *SourceBuilder) Source() []byte {
	return []byte(s.b.String())
}

func (s *SourceBuilder) StringSource() string {
	return s.b.String()
}

func ReadTokenMap(path string) (TokenMap, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	out := TokenMap{}
	s := bufio.NewScanner(f)
	inConflict := false
	takeConflictSide := true
	for s.Scan() {
		line := strings.TrimSpace(s.Text())
		if strings.HasPrefix(line, "<<<<<<<") {
			inConflict = true
			takeConflictSide = true
			continue
		}
		if inConflict && strings.HasPrefix(line, "=======") {
			takeConflictSide = false
			continue
		}
		if inConflict && strings.HasPrefix(line, ">>>>>>>") {
			inConflict = false
			takeConflictSide = true
			continue
		}
		if inConflict && !takeConflictSide {
			continue
		}

		parts := strings.Split(s.Text(), "\t")
		if len(parts) < 2 {
			continue
		}
		name := strings.TrimSpace(parts[0])
		raw, err := hex.DecodeString(strings.TrimSpace(parts[1]))
		if err != nil || len(raw) != HashSize || name == "" {
			continue
		}
		var h Hash
		copy(h[:], raw)
		out[name] = h
	}
	if err := s.Err(); err != nil {
		return nil, err
	}
	return out, nil
}

func CompileFile(path string, tokens TokenMap) ([]byte, error) {
	src, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	return CompileString(path, string(src), tokens)
}

func CompileString(name, src string, tokens TokenMap) ([]byte, error) {
	var out []byte
	s := bufio.NewScanner(strings.NewReader(src))
	s.Buffer(make([]byte, 1024), 16<<20)
	lineNo := 0
	for s.Scan() {
		lineNo++
		rec, ok, err := compileLine(s.Text(), tokens)
		if err != nil {
			return nil, fmt.Errorf("%s:%d: %w", name, lineNo, err)
		}
		if ok {
			out = append(out, rec...)
		}
	}
	if err := s.Err(); err != nil {
		return nil, err
	}
	return append(out, make([]byte, HashSize)...), nil
}

func Record(tok Hash, payload []byte) []byte {
	out := make([]byte, 36+len(payload))
	copy(out, tok[:])
	binary.LittleEndian.PutUint32(out[32:36], uint32(len(payload)+4))
	copy(out[36:], payload)
	return out
}

func HashBytes(data []byte) Hash {
	return sha256.Sum256(data)
}

func compileLine(line string, tokens TokenMap) ([]byte, bool, error) {
	name, kind, rest, ok := splitDirective(line)
	if !ok {
		return nil, false, nil
	}
	tok, found := tokens[name]
	if !found {
		return nil, false, fmt.Errorf("unknown token %q", name)
	}

	payload := []byte(nil)
	if kind != "" {
		var err error
		payload, err = parsePayload(kind, rest)
		if err != nil {
			return nil, false, err
		}
	}
	return Record(tok, payload), true, nil
}

func splitDirective(line string) (name, kind, rest string, ok bool) {
	line = strings.TrimSpace(line)
	if line == "" || strings.HasPrefix(line, "#") || strings.HasPrefix(line, "//") {
		return "", "", "", false
	}

	name, rest, ok = takeField(line)
	if !ok {
		return "", "", "", false
	}
	kind, rest, _ = takeField(rest)
	return name, kind, rest, true
}

func takeField(s string) (field, rest string, ok bool) {
	s = strings.TrimLeft(s, " \t\r\n")
	if s == "" || strings.HasPrefix(s, "#") || strings.HasPrefix(s, "//") {
		return "", "", false
	}
	for i := 0; i < len(s); i++ {
		if s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n' {
			return s[:i], strings.TrimLeft(s[i:], " \t\r\n"), true
		}
	}
	return s, "", true
}

func parsePayload(kind, rest string) ([]byte, error) {
	switch kind {
	case "u64":
		parts := strings.Fields(stripInlineComment(rest))
		if len(parts) != 1 {
			return nil, fmt.Errorf("u64 payload expects one value")
		}
		v, err := strconv.ParseUint(parts[0], 0, 64)
		if err != nil {
			return nil, err
		}
		out := make([]byte, 8)
		binary.LittleEndian.PutUint64(out, v)
		return out, nil
	case "hash":
		parts := strings.Fields(stripInlineComment(rest))
		if len(parts) != 1 {
			return nil, fmt.Errorf("hash payload expects one 64-character hex value")
		}
		raw, err := hex.DecodeString(parts[0])
		if err != nil || len(raw) != HashSize {
			return nil, fmt.Errorf("hash payload expects 32 bytes")
		}
		return raw, nil
	case "hex":
		hexText := strings.Join(strings.Fields(stripInlineComment(rest)), "")
		if hexText == "" {
			return []byte{}, nil
		}
		raw, err := hex.DecodeString(hexText)
		if err != nil {
			return nil, err
		}
		return raw, nil
	case "str", "string", "utf8":
		return parseStringPayload(rest)
	case "raw":
		return []byte(rest), nil
	default:
		return nil, fmt.Errorf("unknown payload kind %q", kind)
	}
}

func parseStringPayload(rest string) ([]byte, error) {
	rest = strings.TrimSpace(rest)
	if rest == "" {
		return []byte{}, nil
	}
	if rest[0] != '"' && rest[0] != '`' && rest[0] != '\'' {
		return []byte(strings.TrimSpace(stripInlineComment(rest))), nil
	}

	quoted, tail, err := quotedPrefix(rest)
	if err != nil {
		return nil, err
	}
	text, err := strconv.Unquote(quoted)
	if err != nil {
		return nil, err
	}
	if strings.TrimSpace(stripInlineComment(tail)) != "" {
		return nil, fmt.Errorf("unexpected content after string payload")
	}
	return []byte(text), nil
}

func quotedPrefix(s string) (quoted, tail string, err error) {
	quote := s[0]
	if quote == '`' {
		for i := 1; i < len(s); i++ {
			if s[i] == '`' {
				return s[:i+1], s[i+1:], nil
			}
		}
		return "", "", fmt.Errorf("unterminated raw string")
	}
	escaped := false
	for i := 1; i < len(s); i++ {
		if escaped {
			escaped = false
			continue
		}
		if s[i] == '\\' {
			escaped = true
			continue
		}
		if s[i] == quote {
			return s[:i+1], s[i+1:], nil
		}
	}
	return "", "", fmt.Errorf("unterminated string")
}

func stripInlineComment(s string) string {
	quote := byte(0)
	escaped := false
	for i := 0; i < len(s); i++ {
		ch := s[i]
		if quote != 0 {
			if escaped {
				escaped = false
				continue
			}
			if ch == '\\' && quote != '`' {
				escaped = true
				continue
			}
			if ch == quote {
				quote = 0
			}
			continue
		}
		if ch == '"' || ch == '`' || ch == '\'' {
			quote = ch
			continue
		}
		if ch == '#' {
			return s[:i]
		}
		if ch == '/' && i+1 < len(s) && s[i+1] == '/' {
			if i == 0 || s[i-1] == ' ' || s[i-1] == '\t' {
				return s[:i]
			}
		}
	}
	return s
}
