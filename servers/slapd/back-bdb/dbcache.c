/* dbcache.c - manage cache of open databases */
/* $OpenLDAP$ */
/*
 * Copyright 1998-2001 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <sys/stat.h>

#include "slap.h"
#include "back-bdb.h"

int
bdb_db_cache(
    Backend	*be,
    const char *name,
	DB **dbout )
{
	int i;
	int rc;
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	struct bdb_db_info *db;
	char *file;

	*dbout = NULL;

	for( i=BDB_NDB; bdb->bi_databases[i]->bdi_name; i++ ) {
		if( !strcmp( bdb->bi_databases[i]->bdi_name, name) ) {
			*dbout = bdb->bi_databases[i]->bdi_db;
			return 0;
		}
	}

	if( i >= BDB_INDICES ) {
		return -1;
	}

	db = (struct bdb_db_info *) ch_calloc(1, sizeof(struct bdb_db_info));

	db->bdi_name = ch_strdup( name );

	rc = db_create( &db->bdi_db, bdb->bi_dbenv, 0 );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_cache: db_create(%s) failed: %s (%d)\n",
			bdb->bi_dbenv_home, db_strerror(rc), rc );
		return rc;
	}

	file = ch_malloc( strlen( name ) + sizeof(BDB_SUFFIX) );
	sprintf( file, "%s" BDB_SUFFIX, name );

	rc = db->bdi_db->open( db->bdi_db,
		file, name,
		DB_BTREE, DB_CREATE|DB_THREAD,
		bdb->bi_dbenv_mode );

	ch_free( file );

	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_cache: db_open(%s) failed: %s (%d)\n",
			name, db_strerror(rc), rc );
		return rc;
	}

	bdb->bi_databases[i] = db;

	*dbout = db->bdi_db;

	return 0;
}
