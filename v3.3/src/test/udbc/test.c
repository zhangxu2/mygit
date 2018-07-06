#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include "pub_db.h"
#include "db_data_type.h"

#define DB_GET_ROWS 10
//#define __DB_SQUERY__
#define __DB_MQUERY__
//#define __DB_INSERT__

sw_dbcfg_t g_dbcfg =
{
	"CSDB", 
	"DB2",
	"CSDB",
	"CSDB",
	"db2inst1",
	"db2inst1",
	"db2",
	0,
};

int main(int argc, const char *argv[])
{
    int colnum = 0;
    int idx = 0;
    int ret = -1;
    int row = 0, i = 0;
    int rows = 0;
    int row_idx = 0;
    int col_idx = 0;
    char col_name[256];
    char alais[256];
    char *result = NULL;
#if defined(__DB_MQUERY__) || defined(__DB_SQUERY__)
    /*const char *sql =  "SELECT * FROM CHANGETAB";*/
    /*const char *sql =  "SELECT rowid as rowid, student.* FROM STUDENT";*/
    const char *sql =  "SELECT rowid as rowid, student.* FROM STUDENT WHERE rowid=x'2F00000000000000000080CA03000000'";
#elif defined(__DB_INSERT__)
    /*const char *sql =  "INSERT INTO STUDENT VALUES(3, 'Lidan', 25)";*/
    const char *sql =  "UPDATE STUDENT SET id=3, name='Lidan', age=25";
#endif

    fprintf(stderr, "[%s][%d] Call pub_db_init()\n", __FILE__, __LINE__);
    ret = pub_db_init(&g_dbcfg);
    if(ret < 0)
    {
        return -1;
    }

    fprintf(stderr, "[%s][%d] Call pub_db_open()\n", __FILE__, __LINE__);
    ret = pub_db_open();
    fprintf(stderr, "[%s][%d] Call pub_db_open()\n", __FILE__, __LINE__);
    if(ret < 0)
    {
        return -1;
    }

    for(i=0; i<2; i++)
    {
#if 1
    #if defined(__DB_SQUERY__)
        colnum = pub_db_squery(sql);
        for(col_idx=1; col_idx<=colnum; col_idx++)
        {
            fprintf(stderr, " %s\n", pub_db_get_data(1, col_idx));
        }
    #elif defined(__DB_MQUERY__)
        colnum = pub_db_mquery(sql, DB_GET_ROWS);
        fprintf(stderr, "[%s][%d] colnum:%d\n", __FILE__, __LINE__, colnum);
        if (colnum > 0)
        {
            fprintf(stderr, "[%s][%d] Call pub_db_fetch()\n", __FILE__, __LINE__);
            rows = pub_db_fetch();
            fprintf(stderr, "[%s][%d] rows:%d\n", __FILE__, __LINE__, rows);
            while(rows > 0)
            {
                for(row_idx=1; row_idx<=rows; row_idx++)
                {
                    row++;
                    fprintf(stderr, "row: %d ",  row);

                    for(col_idx=1; col_idx<=colnum; col_idx++)
                    {
                    #if 1
                        result = pub_db_get_data(row_idx, col_idx);
                        #if 1
                            snprintf(alais, sizeof(alais), "%02d%02d", row_idx, col_idx);
                            pub_db_set_alias(alais, result, strlen(result));
                            fprintf(stderr, "[%s][%d] [%d][%d] : [%s][%s]\n",
                                __FILE__, __LINE__, row, col_idx, pub_db_get_alias(alais), result);
                        #else
                            fprintf(stderr, "col:%d %s\t", col_idx, result);
                        #endif
                    #else
                        fprintf(stderr, " %s ", pub_db_get_data_by_name(row_idx, "age"));
                    #endif
                        }
                    fprintf(stderr, "\n");
                    }
                fprintf(stderr, "[%s][%d] Call pub_db_fetch()\n", __FILE__, __LINE__);
                rows = pub_db_fetch();
                fprintf(stderr, "[%s][%d] rows:%d\n", __FILE__, __LINE__, rows);
            }
        EXIT:
            pub_db_cclose();
        }
    #elif defined(__DB_INSERT__)
        ret = pub_db_nquery(sql);
    #endif
#endif
    }
    pub_db_update_by_rowid("UPDATE student SET name='WANG BA DAN1'", "2F00000000000000000080CA03000000");
    pub_db_update_by_rowid("UPDATE student SET name='WANG BA DAN2'", "2F00000000000000000080CA03000000");
    pub_db_update_by_rowid("UPDATE student SET name='WANG BA DAN3'", "2F00000000000000000080CA03000000");
    pub_db_update_by_rowid("UPDATE student SET name='WANG BA DAN4'", "2F00000000000000000080CA03000000");
    pub_db_update_by_rowid("UPDATE student SET name='WANG BA DAN5'", "2F00000000000000000080CA03000000");
    pub_db_update_by_rowid("UPDATE student SET name='WANG BA DAN6'", "2F00000000000000000080CA03000000");
    fprintf(stderr, "[%s][%d] Call pub_db_close()\n", __FILE__, __LINE__);
    pub_db_close();
    pub_db_release();

    return 0;
}
