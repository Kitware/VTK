/******************************************************************************
 * Project:  PROJ
 * Purpose:  SQLite3 related utilities
 * Author:   Even Rouault, <even.rouault at spatialys.com>
 *
 ******************************************************************************
 * Copyright (c) 2019, Even Rouault, <even.rouault at spatialys.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

#include "sqlite3_utils.hpp"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <cstdlib>
#include <cstring>
#include <sstream> // std::ostringstream

NS_PROJ_START

// ---------------------------------------------------------------------------

SQLite3VFS::SQLite3VFS(pj_sqlite3_vfs *vfs) : vfs_(vfs) {}

// ---------------------------------------------------------------------------

SQLite3VFS::~SQLite3VFS() {
    if (vfs_) {
        sqlite3_vfs_unregister(vfs_);
        delete vfs_;
    }
}

// ---------------------------------------------------------------------------

const char *SQLite3VFS::name() const { return vfs_->namePtr.c_str(); }

// ---------------------------------------------------------------------------

typedef int (*ClosePtr)(sqlite3_file *);

// ---------------------------------------------------------------------------

static int VFSClose(sqlite3_file *file) {
    sqlite3_vfs *defaultVFS = sqlite3_vfs_find(nullptr);
    assert(defaultVFS);
    ClosePtr defaultClosePtr;
    std::memcpy(&defaultClosePtr,
                reinterpret_cast<char *>(file) + defaultVFS->szOsFile,
                sizeof(ClosePtr));
    void *methods = const_cast<sqlite3_io_methods *>(file->pMethods);
    int ret = defaultClosePtr(file);
    std::free(methods);
    return ret;
}

// ---------------------------------------------------------------------------

static int VSFNoOpLockUnlockSync(sqlite3_file *, int) { return SQLITE_OK; }

// ---------------------------------------------------------------------------

static int VFSCustomOpen(sqlite3_vfs *vfs, const char *name, sqlite3_file *file,
                         int flags, int *outFlags) {
    auto realVFS = static_cast<pj_sqlite3_vfs *>(vfs);
    sqlite3_vfs *defaultVFS = static_cast<sqlite3_vfs *>(vfs->pAppData);
    int ret = defaultVFS->xOpen(defaultVFS, name, file, flags, outFlags);
    if (ret == SQLITE_OK) {
        ClosePtr defaultClosePtr = file->pMethods->xClose;
        assert(defaultClosePtr);
        sqlite3_io_methods *methods = static_cast<sqlite3_io_methods *>(
            std::malloc(sizeof(sqlite3_io_methods)));
        if (!methods) {
            file->pMethods->xClose(file);
            return SQLITE_NOMEM;
        }
        memcpy(methods, file->pMethods, sizeof(sqlite3_io_methods));
        methods->xClose = VFSClose;
        if (realVFS->fakeSync) {
            // Disable xSync because it can be significantly slow and we don't
            // need
            // that level of data integrity guarantee for the cache.
            methods->xSync = VSFNoOpLockUnlockSync;
        }
        if (realVFS->fakeLock) {
            methods->xLock = VSFNoOpLockUnlockSync;
            methods->xUnlock = VSFNoOpLockUnlockSync;
        }
        file->pMethods = methods;
        // Save original xClose pointer at end of file structure
        std::memcpy(reinterpret_cast<char *>(file) + defaultVFS->szOsFile,
                    &defaultClosePtr, sizeof(ClosePtr));
    }
    return ret;
}

// ---------------------------------------------------------------------------

static int VFSCustomAccess(sqlite3_vfs *vfs, const char *zName, int flags,
                           int *pResOut) {
    sqlite3_vfs *defaultVFS = static_cast<sqlite3_vfs *>(vfs->pAppData);
    // Do not bother stat'ing for journal or wal files
    if (std::strstr(zName, "-journal") || std::strstr(zName, "-wal")) {
        *pResOut = false;
        return SQLITE_OK;
    }
    return defaultVFS->xAccess(defaultVFS, zName, flags, pResOut);
}

// ---------------------------------------------------------------------------

// SQLite3 logging infrastructure
static void projSqlite3LogCallback(void *, int iErrCode, const char *zMsg) {
    fprintf(stderr, "SQLite3 message: (code %d) %s\n", iErrCode, zMsg);
}

std::unique_ptr<SQLite3VFS> SQLite3VFS::create(bool fakeSync, bool fakeLock,
                                               bool skipStatJournalAndWAL) {

    // Install SQLite3 logger if PROJ_LOG_SQLITE3 env var is defined
    struct InstallSqliteLogger {
        InstallSqliteLogger() {
            if (getenv("PROJ_LOG_SQLITE3") != nullptr) {
                sqlite3_config(SQLITE_CONFIG_LOG, projSqlite3LogCallback,
                               nullptr);
            }
        }
    };
    static InstallSqliteLogger installSqliteLogger;

    // Call to sqlite3_initialize() is normally not needed, except for
    // people building SQLite3 with -DSQLITE_OMIT_AUTOINIT
    sqlite3_initialize();
    sqlite3_vfs *defaultVFS = sqlite3_vfs_find(nullptr);
    assert(defaultVFS);

    auto vfs = new pj_sqlite3_vfs();
    vfs->fakeSync = fakeSync;
    vfs->fakeLock = fakeLock;

    auto vfsUnique = std::unique_ptr<SQLite3VFS>(new SQLite3VFS(vfs));

    std::ostringstream buffer;
    buffer << vfs;
    vfs->namePtr = buffer.str();

    vfs->iVersion = 1;
    vfs->szOsFile = defaultVFS->szOsFile + sizeof(ClosePtr);
    vfs->mxPathname = defaultVFS->mxPathname;
    vfs->zName = vfs->namePtr.c_str();
    vfs->pAppData = defaultVFS;
    vfs->xOpen = VFSCustomOpen;
    vfs->xDelete = defaultVFS->xDelete;
    vfs->xAccess =
        skipStatJournalAndWAL ? VFSCustomAccess : defaultVFS->xAccess;
    vfs->xFullPathname = defaultVFS->xFullPathname;
    vfs->xDlOpen = defaultVFS->xDlOpen;
    vfs->xDlError = defaultVFS->xDlError;
    vfs->xDlSym = defaultVFS->xDlSym;
    vfs->xDlClose = defaultVFS->xDlClose;
    vfs->xRandomness = defaultVFS->xRandomness;
    vfs->xSleep = defaultVFS->xSleep;
    vfs->xCurrentTime = defaultVFS->xCurrentTime;
    vfs->xGetLastError = defaultVFS->xGetLastError;
    vfs->xCurrentTimeInt64 = defaultVFS->xCurrentTimeInt64;
    if (sqlite3_vfs_register(vfs, false) == SQLITE_OK) {
        return vfsUnique;
    }
    delete vfsUnique->vfs_;
    vfsUnique->vfs_ = nullptr;
    return nullptr;
}

// ---------------------------------------------------------------------------

SQLiteStatement::SQLiteStatement(sqlite3_stmt *hStmtIn) : hStmt(hStmtIn) {}

// ---------------------------------------------------------------------------

NS_PROJ_END
