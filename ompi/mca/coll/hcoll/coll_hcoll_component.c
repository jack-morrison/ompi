/**
  Copyright (c) 2011 Mellanox Technologies. All rights reserved.
  $COPYRIGHT$

  Additional copyrights may follow

  $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include <dlfcn.h>
#include <libgen.h>

#include "coll_hcoll.h"
#include "opal/mca/installdirs/installdirs.h"

/*
 * Public string showing the coll ompi_hcol component version number
 */
const char *mca_coll_hcoll_component_version_string =
  "Open MPI HCOL collective MCA component version " OMPI_VERSION;


static int hcoll_open(void);
static int hcoll_close(void);
static int hcoll_register(void);
int mca_coll_hcoll_output = -1;
mca_coll_hcoll_component_t mca_coll_hcoll_component = {

    /* First, the mca_component_t struct containing meta information
       about the component itfca */
    {
        {
            MCA_COLL_BASE_VERSION_2_0_0,

            /* Component name and version */
            "hcoll",
            OMPI_MAJOR_VERSION,
            OMPI_MINOR_VERSION,
            OMPI_RELEASE_VERSION,

            /* Component open and close functions */
            hcoll_open,
            hcoll_close,
            NULL,
            hcoll_register
        },
        {
            /* The component is not checkpoint ready */
            MCA_BASE_METADATA_PARAM_NONE
        },

        /* Initialization / querying functions */

        mca_coll_hcoll_init_query,
        mca_coll_hcoll_comm_query,
    },
    90,
    0,
    "",
    1
};




int mca_coll_hcoll_get_lib(void)
{

    memset(&mca_coll_hcoll_component.hcoll_ops,
            0, sizeof(mca_coll_hcoll_component.hcoll_ops));

    return OMPI_SUCCESS;
}

static void mca_coll_hcoll_close_lib(void)
{

}

/*
 *  * Local flags
 *   */
enum {
    REGINT_NEG_ONE_OK = 0x01,
    REGINT_GE_ZERO = 0x02,
    REGINT_GE_ONE = 0x04,
    REGINT_NONZERO = 0x08,
    REGINT_MAX = 0x88
};

enum {
    REGSTR_EMPTY_OK = 0x01,
    REGSTR_MAX = 0x88
};



/*
 * utility routine for string parameter registration
 */
static int reg_string(const char* param_name,
        const char* deprecated_param_name,
        const char* param_desc,
        const char* default_value, char **storage,
        int flags)
{
    int index;

    *storage = (char *) default_value;
    index = mca_base_component_var_register(
            &mca_coll_hcoll_component.super.collm_version,
            param_name, param_desc, MCA_BASE_VAR_TYPE_STRING,
            NULL, 0, 0, OPAL_INFO_LVL_9,
            MCA_BASE_VAR_SCOPE_READONLY, storage);
    if (NULL != deprecated_param_name) {
        (void) mca_base_var_register_synonym(index,
                "ompi", "coll", "hcoll", deprecated_param_name,
                MCA_BASE_VAR_SYN_FLAG_DEPRECATED);
    }

    if (0 != (flags & REGSTR_EMPTY_OK) &&
            (NULL == *storage || 0 == strlen(*storage))) {
        opal_output(0, "Bad parameter value for parameter \"%s\"",
                param_name);
        return OMPI_ERR_BAD_PARAM;
    }

    return OMPI_SUCCESS;
}



/*
 * Utility routine for integer parameter registration
 */
static int reg_int(const char* param_name,
        const char* deprecated_param_name,
        const char* param_desc,
        int default_value, int *storage, int flags)
{
    int index;

    *storage = default_value;
    index = mca_base_component_var_register(
            &mca_coll_hcoll_component.super.collm_version,
            param_name, param_desc, MCA_BASE_VAR_TYPE_INT,
            NULL, 0, 0,OPAL_INFO_LVL_9,
            MCA_BASE_VAR_SCOPE_READONLY, storage);
    if (NULL != deprecated_param_name) {
        (void) mca_base_var_register_synonym(index,
                "ompi", "coll", "hcoll", deprecated_param_name,
                MCA_BASE_VAR_SYN_FLAG_DEPRECATED);
    }

    if (0 != (flags & REGINT_NEG_ONE_OK) && -1 == *storage) {
        return OMPI_SUCCESS;
    }

    if ((0 != (flags & REGINT_GE_ZERO) && *storage < 0) ||
            (0 != (flags & REGINT_GE_ONE) && *storage < 1) ||
            (0 != (flags & REGINT_NONZERO) && 0 == *storage)) {
        opal_output(0, "Bad parameter value for parameter \"%s\"",
                param_name);
        return OMPI_ERR_BAD_PARAM;
    }

    return OMPI_SUCCESS;
}


static int hcoll_register(void)
{

    int ret, tmp;

    ret = OMPI_SUCCESS;

#define CHECK(expr) do {                    \
            tmp = (expr);                       \
            if (OMPI_SUCCESS != tmp) ret = tmp; \
         } while (0)



    CHECK(reg_int("priority",NULL,
                "Priority of the hcol coll component",
                           90,
                           &mca_coll_hcoll_component.hcoll_priority,
                           0));

    CHECK(reg_int("verbose", NULL,
                "Verbose level of the hcol coll component",
                           0,
                           &mca_coll_hcoll_component.hcoll_verbose,
                           0));

    CHECK(reg_int("enable",NULL,
                           "[1|0|] Enable/Disable HCOL",
                           1 /*enable by default*/,
                           &mca_coll_hcoll_component.hcoll_enable,
                           0));

    CHECK(reg_int("datatype_fallback",NULL,
                           "[1|0|] Enable/Disable user defined dattypes fallback",
                           1 /*enable by default*/,
                           &mca_coll_hcoll_component.hcoll_datatype_fallback,
                           0));

    CHECK(reg_string("library_path", NULL,
                           "HCOL /path/to/libhcol.so",
                           ""COLL_HCOLL_HOME"/libhcol.so",
                           &mca_coll_hcoll_component.hcoll_lib_path,
                           0));

    return ret;
}

static int hcoll_open(void)
{
    int rc;

    mca_coll_hcoll_output = opal_output_open(NULL);
    opal_output_set_verbosity(mca_coll_hcoll_output, mca_coll_hcoll_component.hcoll_verbose);

    hcoll_rte_fns_setup();

    OBJ_CONSTRUCT(&mca_coll_hcoll_component.active_modules,
                  opal_list_t);

    mca_coll_hcoll_component.progress_lock = -1;
    return OMPI_SUCCESS;
}

static int hcoll_close(void)
{
    int rc;
    HCOL_VERBOSE(5,"HCOLL FINALIZE");
    rc = hcoll_finalize();
    opal_progress_unregister(mca_coll_hcoll_progress);
    OBJ_DESTRUCT(&mca_coll_hcoll_component.active_modules);

    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(1,"Hcol library finalize failed");
        return OMPI_ERROR;
    }
    return OMPI_SUCCESS;
}
