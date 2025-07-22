# Database Connectivity Layer (Base/SDBC) Architecture

## Overview

LibreOffice Base provides database functionality through SDBC (Star Database Connectivity), which is modeled after JDBC but implemented in C++ with UNO interfaces. The database layer supports multiple database backends, provides a unified API for database access, and integrates with other LibreOffice applications for data-aware forms and reports.

## Architecture Overview

### Layer Structure

```
LibreOffice Applications (Writer, Calc, etc.)
        ↓
Base UI / Forms / Reports
        ↓
SDBC API Layer (UNO Interfaces)
        ↓
Driver Manager
        ↓
Database Drivers
    ├── Native Drivers (HSQLDB, Firebird)
    ├── ODBC Bridge
    ├── JDBC Bridge
    ├── ADO Bridge (Windows)
    └── Direct Drivers (MySQL, PostgreSQL)
        ↓
External Database Systems
```

## SDBC API

### Core Interfaces

```cpp
// Connection management
interface XDriverManager : XInterface
{
    XConnection getConnection(string url);
    XConnection getConnectionWithInfo(string url, 
                                     sequence<PropertyValue> info);
    void setLoginTimeout(long seconds);
    long getLoginTimeout();
};

// Database connection
interface XConnection : XCloseable
{
    XStatement createStatement();
    XPreparedStatement prepareStatement(string sql);
    XCallableStatement prepareCall(string sql);
    string nativeSQL(string sql);
    void setAutoCommit(boolean autoCommit);
    boolean getAutoCommit();
    void commit();
    void rollback();
    boolean isClosed();
    XDatabaseMetaData getMetaData();
    void setReadOnly(boolean readOnly);
    boolean isReadOnly();
    void setCatalog(string catalog);
    string getCatalog();
    void setTransactionIsolation(long level);
    long getTransactionIsolation();
    XNameAccess getTypeMap();
    void setTypeMap(XNameAccess typeMap);
};
```

### Statement Execution

```cpp
// Basic statement
interface XStatement : XInterface
{
    XResultSet executeQuery(string sql);
    long executeUpdate(string sql);
    boolean execute(string sql);
    XConnection getConnection();
    // ... more methods
};

// Prepared statement
interface XPreparedStatement : XStatement
{
    XResultSet executeQuery();
    long executeUpdate();
    boolean execute();
    void addBatch();
    void clearBatch();
    sequence<long> executeBatch();
    void clearParameters();
    // Parameter setting methods
    void setNull(long parameterIndex, long sqlType);
    void setBoolean(long parameterIndex, boolean x);
    void setShort(long parameterIndex, short x);
    void setInt(long parameterIndex, long x);
    void setLong(long parameterIndex, hyper x);
    void setFloat(long parameterIndex, float x);
    void setDouble(long parameterIndex, double x);
    void setString(long parameterIndex, string x);
    void setBytes(long parameterIndex, sequence<byte> x);
    void setDate(long parameterIndex, Date x);
    void setTime(long parameterIndex, Time x);
    void setTimestamp(long parameterIndex, DateTime x);
    // ... more setter methods
};
```

### Result Sets

```cpp
// Result set interface
interface XResultSet : XInterface
{
    boolean next();
    boolean isBeforeFirst();
    boolean isAfterLast();
    boolean isFirst();
    boolean isLast();
    void beforeFirst();
    void afterLast();
    boolean first();
    boolean last();
    long getRow();
    boolean absolute(long row);
    boolean relative(long rows);
    boolean previous();
    void refreshRow();
    boolean rowUpdated();
    boolean rowInserted();
    boolean rowDeleted();
    XInterface getStatement();
};

// Result set data access
interface XRow : XInterface
{
    boolean wasNull();
    string getString(long columnIndex);
    boolean getBoolean(long columnIndex);
    byte getByte(long columnIndex);
    short getShort(long columnIndex);
    long getInt(long columnIndex);
    hyper getLong(long columnIndex);
    float getFloat(long columnIndex);
    double getDouble(long columnIndex);
    sequence<byte> getBytes(long columnIndex);
    Date getDate(long columnIndex);
    Time getTime(long columnIndex);
    DateTime getTimestamp(long columnIndex);
    XInputStream getBinaryStream(long columnIndex);
    XInputStream getCharacterStream(long columnIndex);
    any getObject(long columnIndex, XNameAccess typeMap);
    XRef getRef(long columnIndex);
    XBlob getBlob(long columnIndex);
    XClob getClob(long columnIndex);
    XArray getArray(long columnIndex);
};
```

## Database Drivers

### Driver Architecture

```cpp
// Driver interface
interface XDriver : XInterface
{
    XConnection connect(string url, sequence<PropertyValue> info);
    boolean acceptsURL(string url);
    sequence<DriverPropertyInfo> getPropertyInfo(string url, 
                                                 sequence<PropertyValue> info);
    long getMajorVersion();
    long getMinorVersion();
};

// Driver implementation base
class ODriver : public XDriver, public XServiceInfo
{
protected:
    Reference<XComponentContext> m_xContext;
    
public:
    // URL format: sdbc:drivername:parameters
    virtual sal_Bool SAL_CALL acceptsURL(const OUString& url) override
    {
        return url.startsWith("sdbc:mydriver:");
    }
    
    virtual Reference<XConnection> SAL_CALL connect(
        const OUString& url,
        const Sequence<PropertyValue>& info) override
    {
        if (!acceptsURL(url))
            return nullptr;
            
        // Parse connection string
        OUString sPath = parseURL(url);
        
        // Create connection
        return new OConnection(this, sPath, info);
    }
};
```

### Native Embedded Databases

#### HSQLDB Driver

```cpp
// HSQLDB embedded database
namespace connectivity { namespace hsqldb {

class OHSQLDriver : public ODriver_BASE
{
    // Embedded Java-based database
    Reference<XInterface> m_xStorageFactory;
    Reference<XDriver> m_xDriver;  // Java driver
    
    Reference<XConnection> connect(const OUString& url,
                                  const Sequence<PropertyValue>& info) override
    {
        // Extract storage from URL
        Reference<XStorage> xStorage = extractStorage(url);
        
        // Start embedded HSQLDB
        startHSQLDB(xStorage);
        
        // Create connection through Java bridge
        return m_xDriver->connect(translateURL(url), info);
    }
};

}} // namespace connectivity::hsqldb
```

#### Firebird Driver

```cpp
// Firebird embedded database (newer default)
namespace connectivity { namespace firebird {

class FirebirdDriver : public ODriver_BASE
{
    // Native C++ embedded database
    Reference<XConnection> connect(const OUString& url,
                                  const Sequence<PropertyValue>& info) override
    {
        OUString sPath = extractPath(url);
        
        // Create/open Firebird database
        isc_db_handle aDBHandle = 0;
        ISC_STATUS_ARRAY status;
        
        if (isc_attach_database(status, 0, sPath.getStr(), 
                               &aDBHandle, 0, nullptr))
            checkStatus(status);
            
        return new Connection(aDBHandle, url);
    }
};

}} // namespace connectivity::firebird
```

### ODBC Bridge

```cpp
namespace connectivity { namespace odbc {

class ODBCDriver : public ODriver_BASE
{
    Reference<XConnection> connect(const OUString& url,
                                  const Sequence<PropertyValue>& info) override
    {
        // Parse ODBC connection string
        OUString sDSN = extractDSN(url);
        
        SQLHANDLE hEnv, hDbc;
        
        // Allocate environment
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, 
                     (SQLPOINTER)SQL_OV_ODBC3, 0);
        
        // Allocate connection
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        
        // Connect
        SQLRETURN ret = SQLConnect(hDbc,
            (SQLCHAR*)sDSN.getStr(), SQL_NTS,
            (SQLCHAR*)sUser.getStr(), SQL_NTS,
            (SQLCHAR*)sPassword.getStr(), SQL_NTS);
            
        if (!SQL_SUCCEEDED(ret))
            throwSQLException(hDbc);
            
        return new OConnection(hDbc, this);
    }
};

}} // namespace connectivity::odbc
```

### JDBC Bridge

```cpp
namespace connectivity { namespace jdbc {

class java_sql_Driver : public ODriver_BASE
{
    // Bridge to Java JDBC drivers
    Reference<XConnection> connect(const OUString& url,
                                  const Sequence<PropertyValue>& info) override
    {
        // Load Java driver class
        OUString sDriverClass = extractDriverClass(info);
        
        SDBThreadAttach t;
        jclass driverClass = t.pEnv->FindClass(
            OUStringToOString(sDriverClass, RTL_TEXTENCODING_ASCII_US).getStr());
            
        // Get driver instance
        jmethodID mid = t.pEnv->GetStaticMethodID(
            driverClass, "getDriver", "()Ljava/sql/Driver;");
        jobject driver = t.pEnv->CallStaticObjectMethod(driverClass, mid);
        
        // Create connection
        return new java_sql_Connection(driver, url, info);
    }
};

}} // namespace connectivity::jdbc
```

## Base Application

### Database Document

```cpp
// Base document implementation
class ODataSource : public ModelDependentComponent,
                   public XDataSource,
                   public XFlushable
{
    // Connection pooling
    Reference<XConnection> getConnection(const OUString& user,
                                       const OUString& password) override
    {
        // Check connection pool
        if (m_pConnectionPool)
        {
            Reference<XConnection> xPooled = 
                m_pConnectionPool->getConnection(m_sURL, user, password);
            if (xPooled.is())
                return xPooled;
        }
        
        // Create new connection
        Reference<XConnection> xNewConn = 
            m_xDriver->connect(m_sURL, createInfo(user, password));
            
        // Add to pool
        if (m_pConnectionPool)
            m_pConnectionPool->addConnection(xNewConn);
            
        return xNewConn;
    }
    
    // Flush changes
    void flush() override
    {
        // Save embedded database
        if (m_xEmbeddedStorage.is())
        {
            m_xEmbeddedStorage->commit();
        }
    }
};
```

### Forms Integration

```cpp
// Data-aware form controls
namespace frm {

class ODatabaseForm : public OFormComponents,
                     public XLoadable,
                     public XRowSetApproveBroadcaster
{
    // Data binding
    Reference<XRowSet> m_xRowSet;
    OUString m_sDataSourceName;
    OUString m_sCommand;
    sal_Int32 m_nCommandType;
    
    // Load data
    void load() override
    {
        // Create row set
        m_xRowSet = createRowSet();
        
        // Configure
        Reference<XPropertySet> xProps(m_xRowSet, UNO_QUERY);
        xProps->setPropertyValue("DataSourceName", Any(m_sDataSourceName));
        xProps->setPropertyValue("Command", Any(m_sCommand));
        xProps->setPropertyValue("CommandType", Any(m_nCommandType));
        
        // Execute
        Reference<XRowSet> xRowSet(m_xRowSet, UNO_QUERY);
        xRowSet->execute();
        
        // Bind controls
        bindControls();
    }
};

} // namespace frm
```

### Query Designer

```cpp
namespace dbaui {

class OQueryDesignView : public ODataView
{
    // Visual query builder
    std::unique_ptr<OSelectionBrowseBox> m_pSelectionBox;
    std::unique_ptr<OTableWindow> m_pTableView;
    std::unique_ptr<OConnectionLine> m_pConnections;
    
    // Generate SQL
    OUString getSQL() const
    {
        OUStringBuffer sql;
        sql.append("SELECT ");
        
        // Add fields
        for (const auto& field : m_pSelectionBox->getFields())
        {
            if (field.bSelected)
                sql.append(field.sName).append(", ");
        }
        
        sql.setLength(sql.getLength() - 2); // Remove last comma
        sql.append(" FROM ");
        
        // Add tables
        for (const auto& table : m_pTableView->getTables())
        {
            sql.append(table->getName()).append(", ");
        }
        
        // Add joins
        for (const auto& conn : m_pConnections->getConnections())
        {
            sql.append(" JOIN ").append(conn->getSQL());
        }
        
        // Add WHERE clause
        OUString sWhere = m_pSelectionBox->getWhereClause();
        if (!sWhere.isEmpty())
            sql.append(" WHERE ").append(sWhere);
            
        return sql.makeStringAndClear();
    }
};

} // namespace dbaui
```

## Connection Pooling

### Pool Implementation

```cpp
class OConnectionPool
{
    struct PooledConnection
    {
        Reference<XConnection> xConnection;
        OUString sUser;
        DateTime aLastAccess;
        bool bInUse;
    };
    
    std::vector<PooledConnection> m_aConnections;
    sal_Int32 m_nMaxConnections;
    sal_Int32 m_nTimeout;
    
public:
    Reference<XConnection> getConnection(const OUString& url,
                                       const OUString& user,
                                       const OUString& password)
    {
        std::lock_guard<std::mutex> aGuard(m_aMutex);
        
        // Find available connection
        for (auto& pooled : m_aConnections)
        {
            if (!pooled.bInUse && 
                pooled.sUser == user &&
                !isExpired(pooled))
            {
                pooled.bInUse = true;
                pooled.aLastAccess = DateTime::now();
                
                // Test connection
                if (isValid(pooled.xConnection))
                    return pooled.xConnection;
                else
                    removeConnection(pooled);
            }
        }
        
        // Create new if under limit
        if (m_aConnections.size() < m_nMaxConnections)
        {
            Reference<XConnection> xNew = createConnection(url, user, password);
            m_aConnections.push_back({xNew, user, DateTime::now(), true});
            return xNew;
        }
        
        // Wait for available connection
        return waitForConnection();
    }
    
    void returnConnection(const Reference<XConnection>& xConnection)
    {
        std::lock_guard<std::mutex> aGuard(m_aMutex);
        
        for (auto& pooled : m_aConnections)
        {
            if (pooled.xConnection == xConnection)
            {
                pooled.bInUse = false;
                pooled.aLastAccess = DateTime::now();
                break;
            }
        }
    }
};
```

## Data Types and Conversion

### Type Mapping

```cpp
// SDBC to UNO type mapping
class DataTypeConversion
{
    static Any toAny(const Reference<XRow>& xRow, 
                    sal_Int32 nColumnIndex,
                    sal_Int32 nSQLType)
    {
        if (xRow->wasNull())
            return Any();
            
        switch (nSQLType)
        {
            case DataType::VARCHAR:
            case DataType::CHAR:
            case DataType::LONGVARCHAR:
                return Any(xRow->getString(nColumnIndex));
                
            case DataType::NUMERIC:
            case DataType::DECIMAL:
                return Any(xRow->getDouble(nColumnIndex));
                
            case DataType::BIT:
            case DataType::BOOLEAN:
                return Any(xRow->getBoolean(nColumnIndex));
                
            case DataType::TINYINT:
            case DataType::SMALLINT:
                return Any(xRow->getShort(nColumnIndex));
                
            case DataType::INTEGER:
                return Any(xRow->getInt(nColumnIndex));
                
            case DataType::BIGINT:
                return Any(xRow->getLong(nColumnIndex));
                
            case DataType::REAL:
            case DataType::FLOAT:
                return Any(xRow->getFloat(nColumnIndex));
                
            case DataType::DOUBLE:
                return Any(xRow->getDouble(nColumnIndex));
                
            case DataType::BINARY:
            case DataType::VARBINARY:
            case DataType::LONGVARBINARY:
                return Any(xRow->getBytes(nColumnIndex));
                
            case DataType::DATE:
                return Any(xRow->getDate(nColumnIndex));
                
            case DataType::TIME:
                return Any(xRow->getTime(nColumnIndex));
                
            case DataType::TIMESTAMP:
                return Any(xRow->getTimestamp(nColumnIndex));
                
            default:
                return Any(xRow->getString(nColumnIndex));
        }
    }
};
```

### Database Metadata

```cpp
// Metadata interface implementation
class ODatabaseMetaData : public XDatabaseMetaData2
{
    Reference<XConnection> m_xConnection;
    
    // Catalog functions
    Reference<XResultSet> getTables(const Any& catalog,
                                   const OUString& schemaPattern,
                                   const OUString& tableNamePattern,
                                   const Sequence<OUString>& types) override
    {
        OUStringBuffer sql;
        sql.append("SELECT TABLE_CAT, TABLE_SCHEM, TABLE_NAME, "
                  "TABLE_TYPE, REMARKS FROM INFORMATION_SCHEMA.TABLES WHERE 1=1");
                  
        if (catalog.hasValue())
            sql.append(" AND TABLE_CAT = ?");
        if (!schemaPattern.isEmpty())
            sql.append(" AND TABLE_SCHEM LIKE ?");
        if (!tableNamePattern.isEmpty())
            sql.append(" AND TABLE_NAME LIKE ?");
            
        Reference<XPreparedStatement> xStmt = 
            m_xConnection->prepareStatement(sql.makeStringAndClear());
            
        sal_Int32 nParam = 1;
        if (catalog.hasValue())
            xStmt->setString(nParam++, catalog.get<OUString>());
        if (!schemaPattern.isEmpty())
            xStmt->setString(nParam++, schemaPattern);
        if (!tableNamePattern.isEmpty())
            xStmt->setString(nParam++, tableNamePattern);
            
        return xStmt->executeQuery();
    }
    
    // Many more metadata methods...
};
```

## Transactions

### Transaction Management

```cpp
class TransactionManager
{
    Reference<XConnection> m_xConnection;
    sal_Int32 m_nTransactionIsolation;
    bool m_bAutoCommit;
    
    // Savepoint support
    class Savepoint
    {
        OUString m_sName;
        Reference<XConnection> m_xConnection;
        
    public:
        Savepoint(const Reference<XConnection>& xConn, const OUString& name)
            : m_sName(name), m_xConnection(xConn)
        {
            Reference<XStatement> xStmt = m_xConnection->createStatement();
            xStmt->execute("SAVEPOINT " + m_sName);
        }
        
        void release()
        {
            Reference<XStatement> xStmt = m_xConnection->createStatement();
            xStmt->execute("RELEASE SAVEPOINT " + m_sName);
        }
        
        void rollback()
        {
            Reference<XStatement> xStmt = m_xConnection->createStatement();
            xStmt->execute("ROLLBACK TO SAVEPOINT " + m_sName);
        }
    };
    
    std::stack<std::unique_ptr<Savepoint>> m_aSavepoints;
    
public:
    void beginTransaction()
    {
        m_xConnection->setAutoCommit(false);
    }
    
    void commit()
    {
        m_xConnection->commit();
        m_aSavepoints = std::stack<std::unique_ptr<Savepoint>>();
    }
    
    void rollback()
    {
        m_xConnection->rollback();
        m_aSavepoints = std::stack<std::unique_ptr<Savepoint>>();
    }
    
    void setSavepoint(const OUString& name)
    {
        m_aSavepoints.push(std::make_unique<Savepoint>(m_xConnection, name));
    }
    
    void rollbackToSavepoint()
    {
        if (!m_aSavepoints.empty())
        {
            m_aSavepoints.top()->rollback();
            m_aSavepoints.pop();
        }
    }
};
```

## Performance Optimization

### Prepared Statement Cache

```cpp
class PreparedStatementCache
{
    struct CachedStatement
    {
        OUString sql;
        Reference<XPreparedStatement> statement;
        DateTime lastUsed;
        sal_Int32 useCount;
    };
    
    std::unordered_map<OUString, CachedStatement> m_aCache;
    sal_Int32 m_nMaxSize;
    
    Reference<XPreparedStatement> getPreparedStatement(
        const Reference<XConnection>& xConnection,
        const OUString& sql)
    {
        auto it = m_aCache.find(sql);
        if (it != m_aCache.end())
        {
            it->second.lastUsed = DateTime::now();
            it->second.useCount++;
            return it->second.statement;
        }
        
        // Create new statement
        Reference<XPreparedStatement> xStmt = 
            xConnection->prepareStatement(sql);
            
        // Add to cache
        if (m_aCache.size() >= m_nMaxSize)
            evictLeastUsed();
            
        m_aCache[sql] = {sql, xStmt, DateTime::now(), 1};
        return xStmt;
    }
};
```

### Result Set Caching

```cpp
class CachedResultSet : public XResultSet
{
    std::vector<std::vector<Any>> m_aData;
    std::vector<OUString> m_aColumnNames;
    sal_Int32 m_nCurrentRow;
    
    // Cache entire result set
    void cacheData(const Reference<XResultSet>& xOriginal)
    {
        Reference<XResultSetMetaData> xMeta = 
            xOriginal->getMetaData();
        sal_Int32 nColumns = xMeta->getColumnCount();
        
        // Cache column info
        for (sal_Int32 i = 1; i <= nColumns; ++i)
            m_aColumnNames.push_back(xMeta->getColumnName(i));
            
        // Cache data
        Reference<XRow> xRow(xOriginal, UNO_QUERY);
        while (xOriginal->next())
        {
            std::vector<Any> aRow;
            for (sal_Int32 i = 1; i <= nColumns; ++i)
                aRow.push_back(getValue(xRow, i, xMeta->getColumnType(i)));
            m_aData.push_back(std::move(aRow));
        }
    }
    
    // Navigation
    sal_Bool next() override
    {
        if (m_nCurrentRow < m_aData.size() - 1)
        {
            ++m_nCurrentRow;
            return true;
        }
        return false;
    }
};
```

## Security

### SQL Injection Prevention

```cpp
class SQLSanitizer
{
    // Parameter binding enforcement
    static OUString buildSecureQuery(const OUString& baseQuery,
                                    const std::vector<Any>& params)
    {
        // Never concatenate user input
        // Always use parameter binding
        return baseQuery; // Parameters bound separately
    }
    
    // Validate identifiers
    static bool isValidIdentifier(const OUString& identifier)
    {
        // Check against whitelist of valid characters
        static const OUString sValidChars = 
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
            
        for (sal_Int32 i = 0; i < identifier.getLength(); ++i)
        {
            if (sValidChars.indexOf(identifier[i]) == -1)
                return false;
        }
        return true;
    }
    
    // Quote identifier for dynamic SQL
    static OUString quoteIdentifier(const OUString& identifier)
    {
        if (!isValidIdentifier(identifier))
            throw SQLException("Invalid identifier");
            
        return "\"" + identifier + "\"";
    }
};
```

### Access Control

```cpp
class DatabaseAccessControl
{
    // User permissions
    struct UserPermissions
    {
        bool canSelect;
        bool canInsert;
        bool canUpdate;
        bool canDelete;
        bool canCreateTable;
        bool canDropTable;
        std::set<OUString> allowedTables;
    };
    
    std::map<OUString, UserPermissions> m_aPermissions;
    
    void checkPermission(const OUString& user,
                        const OUString& operation,
                        const OUString& table)
    {
        auto it = m_aPermissions.find(user);
        if (it == m_aPermissions.end())
            throw SQLException("Access denied");
            
        const UserPermissions& perms = it->second;
        
        if (operation == "SELECT" && !perms.canSelect)
            throw SQLException("SELECT permission denied");
        if (operation == "INSERT" && !perms.canInsert)
            throw SQLException("INSERT permission denied");
        // ... check other operations
        
        if (perms.allowedTables.find(table) == perms.allowedTables.end())
            throw SQLException("Access to table denied");
    }
};
```

## Migration Tools

### Database Migration

```cpp
class DatabaseMigration
{
    Reference<XConnection> m_xSource;
    Reference<XConnection> m_xTarget;
    
    void migrateDatabase()
    {
        // Get source metadata
        Reference<XDatabaseMetaData> xSourceMeta = 
            m_xSource->getMetaData();
            
        // Get all tables
        Reference<XResultSet> xTables = 
            xSourceMeta->getTables(Any(), "%", "%", {"TABLE"});
            
        while (xTables->next())
        {
            OUString sTableName = xTables->getString(3);
            migrateTable(sTableName);
        }
    }
    
    void migrateTable(const OUString& tableName)
    {
        // Create table in target
        OUString sCreateSQL = getCreateTableSQL(tableName);
        Reference<XStatement> xStmt = m_xTarget->createStatement();
        xStmt->execute(sCreateSQL);
        
        // Copy data
        OUString sSelectSQL = "SELECT * FROM " + tableName;
        Reference<XStatement> xSourceStmt = m_xSource->createStatement();
        Reference<XResultSet> xSourceRS = xSourceStmt->executeQuery(sSelectSQL);
        
        Reference<XResultSetMetaData> xMeta = xSourceRS->getMetaData();
        sal_Int32 nColumns = xMeta->getColumnCount();
        
        // Prepare insert
        OUStringBuffer sInsertSQL;
        sInsertSQL.append("INSERT INTO ").append(tableName).append(" VALUES (");
        for (sal_Int32 i = 1; i <= nColumns; ++i)
        {
            sInsertSQL.append("?");
            if (i < nColumns)
                sInsertSQL.append(",");
        }
        sInsertSQL.append(")");
        
        Reference<XPreparedStatement> xInsertStmt = 
            m_xTarget->prepareStatement(sInsertSQL.makeStringAndClear());
            
        // Copy rows
        while (xSourceRS->next())
        {
            for (sal_Int32 i = 1; i <= nColumns; ++i)
                copyValue(xSourceRS, xInsertStmt, i);
                
            xInsertStmt->executeUpdate();
        }
    }
};
```

## Future Enhancements

### Planned Features

1. **NoSQL Support**: Document store integration
2. **Cloud Databases**: Native cloud database drivers
3. **Performance**: Query optimization improvements
4. **Security**: Enhanced encryption and access control
5. **Big Data**: Better handling of large datasets

### Architecture Evolution

- Async query execution
- Better connection pooling
- Improved metadata caching
- Native JSON/XML support
- GraphQL integration

---

This documentation covers the LibreOffice database connectivity layer (Base/SDBC) architecture. The system provides a flexible, extensible framework for database access across multiple database systems and platforms.