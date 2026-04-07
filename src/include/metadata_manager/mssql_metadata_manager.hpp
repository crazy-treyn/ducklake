//===----------------------------------------------------------------------===//
// DuckDB
//
// metadata_manager/mssql_metadata_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/ducklake_metadata_manager.hpp"

namespace duckdb {

class MSSQLMetadataManager : public DuckLakeMetadataManager {
public:
	explicit MSSQLMetadataManager(DuckLakeTransaction &transaction);

	static unique_ptr<DuckLakeMetadataManager> Create(DuckLakeTransaction &transaction) {
		return make_uniq<MSSQLMetadataManager>(transaction);
	}

	bool TypeIsNativelySupported(const LogicalType &type) override;
	bool SupportsInlining(const LogicalType &type) override;
	bool SupportsAppender() const override {
		return false;
	}

	string GetColumnTypeInternal(const LogicalType &type) override;
};

} // namespace duckdb
