/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 <ubaldo@eja.it>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "timeout.h"
#include "api_os.h"
#include "api_audio.h"
#include "api_fs.h"
#include "py/runtime.h"

uint8_t audioTimeout = 5;
uint8_t audioVolume = 5;
bool audioPlayEnd = true;


void audioRecordCallback(AUDIO_Error_t result)
{
    if (result != AUDIO_ERROR_NO) {
        Trace(1,"audio record error: %d", result);
    }
}


STATIC mp_obj_t modaudio_record(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Audio Record.
    // Args:
    //     filename (str): output file name with full path
    //     timeout (int): timeout in seconds (default 5)
    //	   channel (int): audio channel (default 2, AUDIO_MODE_LOUDSPEAKER)
    //	   type (int): audio type (default 3, AUDIO_TYPE_AMR)
    //	   mode (int): audio mode (default 7, AUDIO_RECORD_MODE_AMR122)
    // ========================================
    
    int timeout = audioTimeout;
    int channel = AUDIO_MODE_LOUDSPEAKER;
    int type = AUDIO_TYPE_AMR;
    int mode = AUDIO_RECORD_MODE_AMR122;
    const char* filename;
    
    if (n_args > 0) {
     filename = mp_obj_str_get_str(args[0]);
    } else {
        mp_raise_ValueError("filename is mandatory");
        return mp_const_none;
    }
    if (n_args >= 2) {
        timeout = mp_obj_get_int(args[1]); 
    }
    if (n_args >= 3) {
        channel = mp_obj_get_int(args[2]);
    }
    if (n_args >= 3) {
        type = mp_obj_get_int(args[3]);
    }
    if (n_args >= 4) {
        mode = mp_obj_get_int(args[4]);
    }
    Trace(1,"record start, filename: %s, timeout: %d, channel: %d, type: %d, mode: %d", filename, timeout, channel, type, mode);
    int audioRecordFd = API_FS_Open(filename,FS_O_WRONLY|FS_O_CREAT|FS_O_TRUNC,0);
    if(audioRecordFd < 0) {
        Trace(1,"audio open file error");
        mp_raise_ValueError("file error");
        return mp_const_none;
    }
    AUDIO_SetMode(channel);
    AUDIO_Error_t result =  AUDIO_RecordStart(type, mode, audioRecordFd, audioRecordCallback, NULL);
    if (result != AUDIO_ERROR_NO) {
        Trace(1,"record start fail: %d",result);
        API_FS_Close(audioRecordFd);
        audioRecordFd = -1;
        mp_raise_ValueError("cannot start recording");
        return mp_const_none;
    }
    Trace(1,"record start");
    while(--timeout)
    {
        Trace(1,"recording");
        OS_Sleep(1000);
    }
    Trace(1,"record stop");
    AUDIO_RecordStop();
    API_FS_Close(audioRecordFd);
    audioRecordFd = -1;
    Trace(1,"record end");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modaudio_record_obj, 1, 2, modaudio_record);


STATIC mp_obj_t modaudio_record_stop(void) {
    //Stop recording audio
    AUDIO_RecordStop();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_record_stop_obj, modaudio_record_stop);


void audioPlayCallback(AUDIO_Error_t result)
{
    Trace(1,"play callback result: %d",result);
    if (AUDIO_ERROR_END_OF_FILE == result) {
        Trace(1,"play music file end");
    }
    audioPlayEnd = true;
    AUDIO_Stop();
}


STATIC mp_obj_t modaudio_play(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Audio Play.
    // Args:
    //     filename (str): input file name with full path
    //     volume (int):  (default 5)
    //	   channel (int): audio channel (default 2, AUDIO_MODE_LOUDSPEAKER)
    //	   type (int): audio type (default 3, AUDIO_TYPE_AMR)
    // ========================================
    int volume = audioVolume;
    int channel = AUDIO_MODE_LOUDSPEAKER;
    int type = AUDIO_TYPE_AMR;
    const char* filename;
    if (n_args > 0) {
     filename = mp_obj_str_get_str(args[0]);
    } else {
        mp_raise_ValueError("filename is mandatory");
        return mp_const_none;
    }
    if (n_args >= 2) {
        volume = mp_obj_get_int(args[1]); 
    }
    if (n_args >= 3) {
        channel = mp_obj_get_int(args[2]);
    }
    if (n_args >= 3) {
        type = mp_obj_get_int(args[3]);
    }
    Trace(1,"play start, filename: %s, volume: %d, channel: %d, type: %d", filename, volume, channel, type);
    
    AUDIO_Error_t ret = AUDIO_Play(filename, type, audioPlayCallback);
    if( ret != AUDIO_ERROR_NO)
    {
        Trace(1,"play music fail: %d",ret);
        mp_raise_ValueError("cannot play file");        
        return mp_const_none;        
    }
    else
    {
        audioPlayEnd = false;
        Trace(1,"play music success");
    }
    OS_Sleep(200);
    AUDIO_SetMode(channel);
    AUDIO_SpeakerOpen();
    AUDIO_SpeakerSetVolume(volume);

    uint8_t i = AUDIO_EQ_NORMAL;
    while(!audioPlayEnd)
    {
        OS_Sleep(1000);
    }

    Trace(1,"play end");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modaudio_play_obj, 1, 2, modaudio_play);

/*
STATIC mp_obj_t modaudio_pause(void) {
    //Pause audio play
    AUDIO_Pause();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_pause_obj, modaudio_pause);


STATIC mp_obj_t modaudio_resume(void) {
   //Resume playing from pause
   AUDIO_Resume();
   return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_resume_obj, modaudio_resume);
*/

STATIC mp_obj_t modaudio_play_stop(void) {
    //Stop playing audio
    AUDIO_Stop();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_play_stop_obj, modaudio_play_stop);


STATIC const mp_map_elem_t mp_module_audio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_audio) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_record), (mp_obj_t)&modaudio_record_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_record_stop), (mp_obj_t)&modaudio_record_stop_obj },    
    { MP_OBJ_NEW_QSTR(MP_QSTR_play), (mp_obj_t)&modaudio_play_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_play_stop), (mp_obj_t)&modaudio_play_stop_obj },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_audio_globals, mp_module_audio_globals_table);


const mp_obj_module_t audio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_audio_globals,
};
