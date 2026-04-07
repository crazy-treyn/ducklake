<div align="center">
  <picture>
    <source media="(prefers-color-scheme: light)" srcset="../logo/DuckLake_Logo-horizontal.svg">
    <source media="(prefers-color-scheme: dark)" srcset="../logo/DuckLake_Logo-horizontal-dark.svg">
    <img alt="DuckLake logo" src="../logo/DuckLake_Logo-horizontal.svg" height="100">
  </picture>
</div>
<br>

# DuckDB DuckLake Extension

> While we tested the DuckLake extension extensively, it is currently experimental as demonstrated by its version number 0.x.
> If you encounter any problems, please file a [new issue](https://github.com/duckdb/ducklake/issues).

DuckLake is an open Lakehouse format that is built on SQL and Parquet. DuckLake stores metadata in a [catalog database](https://ducklake.select/docs/stable/duckdb/usage/choosing_a_catalog_database), and stores data in Parquet files. The DuckLake extension allows DuckDB to directly read and write data from DuckLake.

See the [DuckLake website](https://ducklake.select) for more information.

## Installation

DuckLake can be installed using the `INSTALL` command:

```sql
INSTALL ducklake;
```

The latest development version can be installed from `core_nightly`:

```sql
FORCE INSTALL ducklake FROM core_nightly;
```

## SQL Server, Azure SQL, and Microsoft Fabric (metadata catalog)

DuckLake can store its metadata in Microsoft SQL Server (including Azure SQL Database and Microsoft Fabric warehouse endpoints) using the community [`mssql` extension](https://duckdb.org/community_extensions/extensions/mssql.html). For **Azure SQL and Fabric**, authentication is typically **Microsoft Entra ID (Azure AD)** via the [`azure` extension](https://duckdb.org/docs/extensions/azure)—not SQL Server `User Id` / `Password`. The mssql extension documents all auth modes in its [AZURE.md](https://github.com/hugr-lab/mssql-extension/blob/main/AZURE.md) guide.

Install what you need:

```sql
INSTALL ducklake;
INSTALL mssql FROM community;
INSTALL azure;
LOAD mssql;
LOAD azure;
```

Sign in with the [Azure CLI](https://learn.microsoft.com/cli/azure/install-azure-cli) (`az login`) and select a subscription (`az account set --subscription ...`) before attaching. DuckLake forwards `META_AZURE_SECRET` and other `META_*` options to the inner `ATTACH ... TYPE mssql` used for the metadata catalog.

### Attach with Azure CLI (recommended for interactive use)

This uses your `az login` session through a **credential_chain** secret (`CHAIN 'cli'`):

```sql
CREATE SECRET azure_cli (
    TYPE azure,
    PROVIDER credential_chain,
    CHAIN 'cli'
);

ATTACH 'ducklake:mssql:Server=myserver.database.windows.net,1433;Database=my_catalog;Encrypt=true' AS my_lake
  (DATA_PATH 'abfss://container@account.dfs.core.windows.net/data/', META_TYPE 'mssql', META_AZURE_SECRET 'azure_cli');

USE my_lake;
```

For Fabric Data Warehouse, use your warehouse host (for example `xyz.datawarehouse.fabric.microsoft.com`) and database name in the connection string the same way.

`META_TYPE 'mssql'` is optional when `METADATA_PATH` / the attach path already uses the `mssql:` prefix; keep it when using DuckLake secrets or a bare connection string.

### Attach with a service principal (automation / CI to Azure)

```sql
CREATE SECRET azure_sp (
    TYPE azure,
    PROVIDER service_principal,
    TENANT_ID 'your-tenant-id',
    CLIENT_ID 'your-client-id',
    CLIENT_SECRET 'your-client-secret'
);

ATTACH 'ducklake:mssql:Server=myserver.database.windows.net,1433;Database=my_catalog;Encrypt=true' AS my_lake
  (DATA_PATH 'abfss://container@account.dfs.core.windows.net/data/', META_TYPE 'mssql', META_AZURE_SECRET 'azure_sp');
```

Grant the app registration access in the database (`CREATE USER ... FROM EXTERNAL PROVIDER`, role membership) as in the mssql extension docs.

### DuckLake `TYPE DUCKLAKE` secrets

Store catalog and data paths in a DuckLake secret and attach by name; reference whichever Azure secret you created (`azure_cli`, `azure_sp`, or another provider):

```sql
CREATE SECRET my_lake (
    TYPE DUCKLAKE,
    METADATA_PATH 'mssql:Server=myserver.database.windows.net,1433;Database=my_catalog;Encrypt=true',
    DATA_PATH 'az://my-data-path/',
    METADATA_PARAMETERS MAP {
        'AZURE_SECRET': 'azure_cli'
    }
);

ATTACH 'ducklake:my_lake' AS my_lake;
```

### SQL authentication (on-premises SQL Server only)

For **self-hosted** SQL Server or a **local Docker** dev container, you can use SQL authentication (`User Id` / `Password`) in the `mssql:` connection string. Do **not** use this pattern for Azure SQL or Fabric; use Entra ID and `AZURE_SECRET` or `ACCESS_TOKEN` as described in [AZURE.md](https://github.com/hugr-lab/mssql-extension/blob/main/AZURE.md).

### Automated tests in this repository

This repo’s CI job runs a **SQL Server 2022 Docker** container with SQL authentication so tests can run without Azure credentials. That setup is **not** representative of Azure SQL or Fabric production auth.

## Usage

DuckLake databases can be attached using the  [`ATTACH`](https://duckdb.org/docs/stable/sql/statements/attach.html) syntax, after which tables can be created, modified and queried using standard SQL.

Below is a short usage example that stores the metadata in a DuckDB database file called `metadata.ducklake`, and the data in Parquet files in the `file_path` directory:

```sql
ATTACH 'ducklake:metadata.ducklake' AS my_ducklake (DATA_PATH 'file_path/');
USE my_ducklake;
CREATE TABLE my_ducklake.my_table(id INTEGER, val VARCHAR);
INSERT INTO my_ducklake.my_table VALUES (1, 'Hello'), (2, 'World');
FROM my_ducklake.my_table;
┌───────┬─────────┐
│  id   │   val   │
│ int32 │ varchar │
├───────┼─────────┤
│     1 │ Hello   │
│     2 │ World   │
└───────┴─────────┘
```
##### Updates
```sql
UPDATE my_ducklake.my_table SET val='DuckLake' WHERE id=2;
FROM my_ducklake.my_table;
┌───────┬──────────┐
│  id   │   val    │
│ int32 │ varchar  │
├───────┼──────────┤
│     1 │ Hello    │
│     2 │ DuckLake │
└───────┴──────────┘
```
##### Time Travel
```sql
FROM my_ducklake.my_table AT (VERSION => 2);
┌───────┬─────────┐
│  id   │   val   │
│ int32 │ varchar │
├───────┼─────────┤
│     1 │ Hello   │
│     2 │ World   │
└───────┴─────────┘
```
##### Schema Evolution
```sql
ALTER TABLE my_ducklake.my_table ADD COLUMN new_column VARCHAR;
FROM my_ducklake.my_table;
┌───────┬──────────┬────────────┐
│  id   │   val    │ new_column │
│ int32 │ varchar  │  varchar   │
├───────┼──────────┼────────────┤
│     1 │ Hello    │ NULL       │
│     2 │ DuckLake │ NULL       │
└───────┴──────────┴────────────┘
```
##### Change Data Feed
```sql
FROM my_ducklake.table_changes('my_table', 2, 2);
┌─────────────┬───────┬─────────────┬───────┬─────────┐
│ snapshot_id │ rowid │ change_type │  id   │   val   │
│    int64    │ int64 │   varchar   │ int32 │ varchar │
├─────────────┼───────┼─────────────┼───────┼─────────┤
│           2 │     0 │ insert      │     1 │ Hello   │
│           2 │     1 │ insert      │     2 │ World   │
└─────────────┴───────┴─────────────┴───────┴─────────┘
```

See the [Usage](https://ducklake.select/docs/stable/duckdb/introduction) guide for more information.

## Building & Loading the Extension

To build, type
```
git submodule init
git submodule update
# to build with multiple cores, use `make GEN=ninja release`
make pull
make
```

To run, run the bundled `duckdb` shell:
```
 ./build/release/duckdb
```
