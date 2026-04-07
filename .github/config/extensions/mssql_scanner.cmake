# MSSQL extension (community): native TDS connector for SQL Server / Azure SQL / Fabric.
# OpenSSL is linked inside the extension; use DONT_LINK like postgres_scanner.
if(NOT MINGW AND NOT ${WASM_ENABLED})
    duckdb_extension_load(mssql
            DONT_LINK
            GIT_URL https://github.com/hugr-lab/mssql-extension
            GIT_TAG v0.1.18
            LOAD_TESTS
    )
endif()
