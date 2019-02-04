#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_info.h"
#include "api_sim.h"

STATIC mp_obj_t get_imei(void) {
    // ========================================
    // Retrieves IMEI number.
    // Returns:
    //     IMEI number as a string.
    // ========================================
    uint8_t imei[16];
    memset(imei,0,sizeof(imei));
    INFO_GetIMEI(imei);
    return mp_obj_new_str(imei, strlen(imei));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_imei_obj, get_imei);

STATIC mp_obj_t is_sim_present(void) {
    // ========================================
    // Checks whether the SIM card is inserted and ICCID can be retrieved.
    // Returns:
    //     True if SIM present.
    // ========================================
    uint8_t iccid[21];
    memset(iccid, 0, sizeof(iccid));
    return mp_obj_new_bool(SIM_GetICCID(iccid));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(is_sim_present_obj, is_sim_present);

STATIC mp_obj_t get_iccid(void) {
    // ========================================
    // Retrieves ICCID number.
    // Returns:
    //     ICCID number as a string.
    // ========================================
    uint8_t iccid[21];
    memset(iccid, 0, sizeof(iccid));
    if (SIM_GetICCID(iccid))
        return mp_obj_new_str(iccid, strlen(iccid));
    else {
        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_iccid_obj, get_iccid);

STATIC const mp_map_elem_t mp_module_cellular_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_cellular) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imei), (mp_obj_t)&get_imei_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_sim_present), (mp_obj_t)&is_sim_present_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_iccid), (mp_obj_t)&get_iccid_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_cellular_globals, mp_module_cellular_globals_table);

const mp_obj_module_t cellular_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_cellular_globals,
};
