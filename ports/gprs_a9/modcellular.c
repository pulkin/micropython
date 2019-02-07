#include "modcellular.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_info.h"
#include "api_sim.h"
#include "api_sms.h"
#include "api_os.h"

#define NTW_REG_BIT 0x01
#define NTW_ROAM_BIT 0x02

// ------
// Notify
// ------

int sim_status = 0;

void notify_no_sim(API_Event_t* event) {
    sim_status = 0;
}

void notify_registered_home(API_Event_t* event) {
    sim_status = NTW_REG_BIT;
}

void notify_registered_roaming(API_Event_t* event) {
    sim_status = NTW_REG_BIT | NTW_ROAM_BIT;
}

// -------
// Methods
// -------

STATIC mp_obj_t get_imei(void) {
    // ========================================
    // Retrieves IMEI number.
    // Returns:
    //     IMEI number as a string.
    // ========================================
    char imei[16];
    memset(imei,0,sizeof(imei));
    INFO_GetIMEI((uint8_t*)imei);
    return mp_obj_new_str(imei, strlen(imei));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_imei_obj, get_imei);

STATIC mp_obj_t is_sim_present(void) {
    // ========================================
    // Checks whether the SIM card is inserted and ICCID can be retrieved.
    // Returns:
    //     True if SIM present.
    // ========================================
    char iccid[21];
    memset(iccid, 0, sizeof(iccid));
    return mp_obj_new_bool(SIM_GetICCID((uint8_t*)iccid));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(is_sim_present_obj, is_sim_present);

STATIC mp_obj_t get_iccid(void) {
    // ========================================
    // Retrieves ICCID number.
    // Returns:
    //     ICCID number as a string.
    // ========================================
    char iccid[21];
    memset(iccid, 0, sizeof(iccid));
    if (SIM_GetICCID((uint8_t*)iccid))
        return mp_obj_new_str(iccid, strlen(iccid));
    else {
        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_iccid_obj, get_iccid);

STATIC mp_obj_t sms_send(mp_obj_t destination, mp_obj_t message) {
    // ========================================
    // Sends an SMS messsage.
    // Args:
    //     destination (str): telephone number;
    //     message (str): message contents;
    // ========================================
    REQUIRES_NETWORK_REGISTRATION

    const char* destination_c = mp_obj_str_get_str(destination);
    const char* message_c = mp_obj_str_get_str(message);

    if (!SMS_SetFormat(SMS_FORMAT_TEXT, SIM0)) {
        mp_raise_ValueError("Failed to set SMS format: is SIM card present?");
        return mp_const_none;
    }

    SMS_Parameter_t smsParam = {
        .fo = 17 , // stadard values
        .vp = 167,
        .pid= 0  ,
        .dcs= 8  , // 0:English 7bit, 4:English 8 bit, 8:Unicode 2 Bytes
    };
    if (!SMS_SetParameter(&smsParam,SIM0)) {
        mp_raise_ValueError("Failed to set SMS parameters");
        return mp_const_none;
    }

    if (!SMS_SetNewMessageStorage(SMS_STORAGE_SIM_CARD)) {
        mp_raise_ValueError("Failed to set SMS storage in the SIM card");
        return mp_const_none;
    }

    uint8_t* unicode = NULL;
    uint32_t unicodeLen;

    if (!SMS_LocalLanguage2Unicode((uint8_t*)message_c, strlen(message_c), CHARSET_UTF_8, &unicode, &unicodeLen)) {
        mp_raise_ValueError("Failed to convert to Unicode before sending SMS");
        return mp_const_none;
    }

    if (!SMS_SendMessage(destination_c, unicode, unicodeLen, SIM0)) {
        OS_Free(unicode);
        mp_raise_ValueError("Failed to send SMS message");
        return mp_const_none;
    }
    OS_Free(unicode);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(sms_send_obj, sms_send);

STATIC const mp_map_elem_t mp_module_cellular_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_cellular) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imei), (mp_obj_t)&get_imei_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_sim_present), (mp_obj_t)&is_sim_present_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_iccid), (mp_obj_t)&get_iccid_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_send), (mp_obj_t)&sms_send_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_cellular_globals, mp_module_cellular_globals_table);

const mp_obj_module_t cellular_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_cellular_globals,
};
