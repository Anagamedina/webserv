# UML: config::utils Namespace

Este diagrama representa las funciones de utilidad dentro del espacio de nombres `config::utils` definidas en [ConfigUtils.hpp](file:///home/daruuu/CLionProjects/webs-daru/src/config/ConfigUtils.hpp).

```mermaid
classDiagram
    class utils {
        <<namespace>>
        +trimLine(string line) string
        +removeComments(string& line) void
        +removeSpacesAndTabs(string& line) string
        +normalizeSpaces(string line) string
        +fileExists(string path) bool
        +split(string str, char delimiter) vector~string~
        +tokenize(string line) vector~string~
        +removeSemicolon(string str) string
        +stringToInt(string str) int
        +exportContentToLogFile(string content, string path) void
        +isValidPath(string path) bool
        +parseSize(string str) long
        +isValidIPv4(string ip) bool
        +isValidHostname(string hostname) bool
        +isValidHost(string host) bool
        +isValidLocationPath(string path) bool
        +isValidHttpMethod(string method) bool
        +checkRootPath(string path) string
        +ensureUploadStorePath(string path) void
    }
    
    namespace config {
        class utils
    }
```

## Cómo visualizarlo
Para ver este diagrama, puedes utilizar:
1. **GitHub/GitLab**: Renderizan Mermaid automáticamente en archivos `.md`.
2. **VS Code**: Con extensiones como "Markdown Preview Mermaid Support".
3. **Editor Online**: Copia el bloque de código en [Mermaid Live Editor](https://mermaid.live/).
