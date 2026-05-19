import os
import re
import sys
import json
import subprocess
from http.server import HTTPServer, SimpleHTTPRequestHandler

PORT = 8000
WORKSPACE_DIR = os.path.dirname(os.path.abspath(__file__))

class KEDRequestHandler(SimpleHTTPRequestHandler):
    def translate_path(self, path):
        # Override to serve files from the 'web' subdirectory
        if path.startswith('/api/'):
            return path
        
        relative_path = path.lstrip('/')
        if not relative_path:
            relative_path = 'index.html'
        
        web_path = os.path.join(WORKSPACE_DIR, 'web', relative_path)
        return web_path

    def do_POST(self):
        if self.path == '/api/compile':
            content_length = int(self.headers['Content-Length'])
            code = self.rfile.read(content_length).decode('utf-8')
            
            # Write to a temporary file
            temp_filepath = os.path.join(WORKSPACE_DIR, 'temp.ked')
            with open(temp_filepath, 'w', encoding='utf-8') as f:
                f.write(code)
            
            # Execute C binary via WSL
            try:
                # We call WSL to run the compiled Linux binary
                result = subprocess.run(
                    ["wsl", "./kde", "temp.ked"],
                    cwd=WORKSPACE_DIR,
                    capture_output=True,
                    text=True,
                    encoding='utf-8',
                    errors='ignore'
                )
                
                stdout = result.stdout
                stderr = result.stderr
                
                # Parse C TUI output
                sections = self.parse_c_output(stdout)
                
                response_data = {
                    "success": True,
                    "ast": sections.get("AST", "(sin AST valido)"),
                    "tac": sections.get("Codigo Intermedio (TAC)", "(sin codigo generado)"),
                    "output": sections.get("Resultado de ejecucion", "(sin variables)"),
                    "symbols": sections.get("Tabla de simbolos", "(vacia)"),
                    "errors": sections.get("Errores", "Sin errores"),
                    "raw_stdout": stdout,
                    "raw_stderr": stderr
                }
            except Exception as e:
                response_data = {
                    "success": False,
                    "error": str(e)
                }
            
            # Clean up temp file
            if os.path.exists(temp_filepath):
                try:
                    os.remove(temp_filepath)
                except:
                    pass
            
            # Send JSON response
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(response_data).encode('utf-8'))
        else:
            self.send_error(404, "API endpoint not found")

    def parse_c_output(self, stdout):
        # Remove ANSI escape sequences
        ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
        clean_stdout = ansi_escape.sub('', stdout)
        
        sections = {}
        current_section = None
        section_lines = []
        
        for line in clean_stdout.split('\n'):
            line_stripped = line.strip()
            # Detect section header
            if line_stripped.startswith("── "):
                # Save previous section if it exists
                if current_section and section_lines:
                    sections[current_section] = "\n".join(section_lines).strip()
                
                # Identify new section
                parts = line_stripped[3:].split('-')
                if parts:
                    current_section = parts[0].strip()
                    section_lines = []
                continue
            
            # Detect section footer
            if line_stripped.startswith("---------") or (line_stripped.startswith("-") and line_stripped.endswith("-") and len(line_stripped) >= 10):
                if current_section:
                    sections[current_section] = "\n".join(section_lines).strip()
                    current_section = None
                continue
            
            if current_section is not None:
                section_lines.append(line)
                
        # Final sweep
        if current_section and section_lines:
            sections[current_section] = "\n".join(section_lines).strip()
            
        return sections

def main():
    print(f"Iniciando Servidor KED Bridge en http://localhost:{PORT} ...")
    print(f"Directorio de Trabajo: {WORKSPACE_DIR}")
    print("Por favor, abre http://localhost:8000 en tu navegador.")
    print("Cuando hagas clic en 'Ejecutar', se invocará tu compilador nativo de C.")
    print("Presiona Ctrl+C para detener el servidor.")
    
    server = HTTPServer(('localhost', PORT), KEDRequestHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor KED Bridge detenido.")
        sys.exit(0)

if __name__ == '__main__':
    main()
