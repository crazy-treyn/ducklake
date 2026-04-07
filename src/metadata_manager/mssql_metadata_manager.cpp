#include "metadata_manager/mssql_metadata_manager.hpp"
#include "common/ducklake_util.hpp"
#include "duckdb/main/database.hpp"
#include "storage/ducklake_catalog.hpp"
#include "storage/ducklake_transaction.hpp"

namespace duckdb {

MSSQLMetadataManager::MSSQLMetadataManager(DuckLakeTransaction &transaction) : DuckLakeMetadataManager(transaction) {
}

bool MSSQLMetadataManager::TypeIsNativelySupported(const LogicalType &type) {
	switch (type.id()) {
	// Unnamed composite types are not supported as SQL Server column types.
	case LogicalTypeId::STRUCT:
	case LogicalTypeId::MAP:
	case LogicalTypeId::LIST:
	case LogicalTypeId::ARRAY:
	// No direct SQL Server equivalent in the mssql extension type mapping.
	case LogicalTypeId::UBIGINT:
	case LogicalTypeId::HUGEINT:
	case LogicalTypeId::UHUGEINT:
	// Prefer VARCHAR round-trip for wide integer / huge types.
	case LogicalTypeId::VARIANT:
	case LogicalTypeId::GEOMETRY:
		return false;
	default:
		return true;
	}
}

bool MSSQLMetadataManager::SupportsInlining(const LogicalType &type) {
	if (type.id() == LogicalTypeId::VARIANT) {
		return false;
	}
	return DuckLakeMetadataManager::SupportsInlining(type);
}

string MSSQLMetadataManager::GetColumnTypeInternal(const LogicalType &column_type) {
	switch (column_type.id()) {
	case LogicalTypeId::DOUBLE:
		return "FLOAT";
	case LogicalTypeId::FLOAT:
		return "REAL";
	case LogicalTypeId::TINYINT:
		return "SMALLINT";
	case LogicalTypeId::UTINYINT:
	case LogicalTypeId::USMALLINT:
		return "INTEGER";
	case LogicalTypeId::UINTEGER:
		return "BIGINT";
	case LogicalTypeId::UBIGINT:
	case LogicalTypeId::HUGEINT:
	case LogicalTypeId::UHUGEINT:
		return "VARCHAR";
	case LogicalTypeId::VARIANT:
		return "VARCHAR";
	default:
		return column_type.ToString();
	}
}

} // namespace duckdb
