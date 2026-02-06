flowchart TD
%% Estilos
classDef done fill:#98c379,stroke:#333,stroke-width:2px,color:black;
classDef partial fill:#e5c07b,stroke:#333,stroke-width:2px,color:black;
classDef pending fill:#e06c75,stroke:#333,stroke-width:2px,color:black;

    Start([ConfigParser::parse]) --> Validations
    
    subgraph Validations [1. Validaciones Previas]
        ValExt[ValidateFileExtension]:::done
        ValPerm[ValidateFilePermissions]:::done
        ValExt --> ValPerm
    end

    Validations --> Cleaning

    subgraph Cleaning [2. Limpieza y Pre-proceso]
        Clean[CleanFileConfig]:::done
        RemoveComm[RemoveComments]:::done
        Trim[TrimLine / Normalize]:::done
        ValBrackets[ValidateCurlyBrackets]:::done
        
        Clean --> RemoveComm --> Trim --> ValBrackets
    end

    Cleaning --> Extraction

    subgraph Extraction [3. Extract blocks]
        Machine[MachineStatesOfConfigFile]:::done
        Extract[extractServerBlock]:::done
        RawVec[Vector: raw_server_blocks_]:::done
        
        Machine -- llama a --> Extract
        Extract -- llena --> RawVec
    end

    Extraction --> Parsing

    subgraph Parsing ["4. Parseo Lógico (Tu punto actual)"]
        Loop[parserServerBlocks]:::partial
        ParseSingle[parseServerBlock]:::partial
        StateMachine{State Machine}:::partial
        
        Loop -- itera raw_blocks --> ParseSingle
        ParseSingle -- usa --> StateMachine
        
        StateMachine -- "IN_SERVER" --> DirServer[Directivas Server]:::partial
        StateMachine -- "IN_LOCATION" --> DirLoc[Directivas Location]:::partial
    end

    Parsing --> Storage

    subgraph Storage [5. Almacenamiento Objetos]
        ServConf[ServerConfig Setters]:::pending
        LocConf[LocationConfig Setters]:::pending
        FinalVec[Vector: servers_]:::pending
        
        DirServer -.-> ServConf
        DirLoc -.-> LocConf
        ServConf --> FinalVec
    end

    %% Leyenda
    subgraph Leyenda
        DONE[Implementado]:::done
        WIP[En Progreso]:::partial
        TODO[Pendiente]:::pending
    end