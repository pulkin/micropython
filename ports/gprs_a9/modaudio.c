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

uint8_t audioVolume = 5;
int audioRecordFd = -1;


STATIC mp_obj_t modaudio_record_start(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Audio Record.
    // Args:
    //     filename (str): output file name with full path
    //	   channel (int): audio channel (default 2, AUDIO_MODE_LOUDSPEAKER)
    //	   type (int): audio type (default 3, AUDIO_TYPE_AMR)
    //	   mode (int): audio mode (default 7, AUDIO_RECORD_MODE_AMR122)
    // ========================================
    
    const char* filename;    
    int channel = AUDIO_MODE_LOUDSPEAKER;
    int type = AUDIO_TYPE_AMR;
    int mode = AUDIO_RECORD_MODE_AMR122;
    
    if (n_args > 0) {
        filename = mp_obj_str_get_str(args[0]);
    } else {
        mp_raise_ValueError("filename is mandatory");
        return mp_const_none;
    }
    if (n_args >= 2) {
        channel = mp_obj_get_int(args[1]);
    }
    if (n_args >= 3) {
        type = mp_obj_get_int(args[2]);
    }
    if (n_args >= 4) {
        mode = mp_obj_get_int(args[3]);
    }
    Trace(1,"audio record start, filename: %s, channel: %d, type: %d, mode: %d", filename, channel, type, mode);
    audioRecordFd = API_FS_Open(filename,FS_O_WRONLY|FS_O_CREAT|FS_O_TRUNC,0);
    if (audioRecordFd < 0) {
        Trace(1,"audio open file error");
        mp_raise_ValueError("file open error");
        return mp_const_none;
    }
    AUDIO_SetMode(channel);
    AUDIO_Error_t result =  AUDIO_RecordStart(type, mode, audioRecordFd, NULL, NULL);
    if (result != AUDIO_ERROR_NO) {
        Trace(1,"audio record start fail: %d",result);
        API_FS_Close(audioRecordFd);
        audioRecordFd = -1;
        mp_raise_ValueError("cannot record audio file");
        return mp_const_none;
    }
    Trace(1,"recording");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modaudio_record_start_obj, 1, 2, modaudio_record_start);


STATIC mp_obj_t modaudio_record_stop(void) {
    //Stop recording audio
    AUDIO_RecordStop();
    API_FS_Close(audioRecordFd);
    audioRecordFd = -1;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_record_stop_obj, modaudio_record_stop);


void audioPlayCallback(AUDIO_Error_t result) {
    Trace(1,"audio play callback result: %d",result);
    AUDIO_Stop();
}


STATIC mp_obj_t modaudio_play_start(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Audio Play.
    // Args:
    //     filename (str): input file name with full path
    //     volume (int):  (default 5)
    //	   channel (int): audio channel (default 2, AUDIO_MODE_LOUDSPEAKER)
    //	   type (int): audio type (default 3, AUDIO_TYPE_AMR)
    // ========================================

    const char* filename;    
    int volume = audioVolume;
    int channel = AUDIO_MODE_LOUDSPEAKER;
    int type = AUDIO_TYPE_AMR;

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
    if (n_args >= 4) {
        type = mp_obj_get_int(args[3]);
    }
    Trace(1,"play start, filename: %s, volume: %d, channel: %d, type: %d", filename, volume, channel, type);
    AUDIO_Error_t result = AUDIO_Play(filename, type, audioPlayCallback);
    if (result != AUDIO_ERROR_NO) {
        Trace(1,"play fail: %d", result);
        mp_raise_ValueError("cannot play audio file");        
        return mp_const_none;        
    } else {
        Trace(1,"playing");
    }
    OS_Sleep(200);
    AUDIO_SetMode(channel);
    AUDIO_SpeakerOpen();
    AUDIO_SpeakerSetVolume(volume);
    Trace(1,"playing");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modaudio_play_start_obj, 1, 2, modaudio_play_start);


STATIC mp_obj_t modaudio_play_pause(void) {
    //Pause audio play
    AUDIO_Pause();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_play_pause_obj, modaudio_play_pause);


STATIC mp_obj_t modaudio_play_resume(void) {
   //Resume playing after pause
   //AUDIO_Resume(fileFd); //?AUDIO_Play doesn't have nor return a fileFd
   return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_play_resume_obj, modaudio_play_resume);


STATIC mp_obj_t modaudio_play_stop(void) {
    //Stop playing audio
    AUDIO_Stop();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(modaudio_play_stop_obj, modaudio_play_stop);


STATIC const mp_map_elem_t mp_module_audio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_audio) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_record_start), (mp_obj_t)&modaudio_record_start_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_record_stop), (mp_obj_t)&modaudio_record_stop_obj },    
    { MP_OBJ_NEW_QSTR(MP_QSTR_play_start), (mp_obj_t)&modaudio_play_start_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_play_stop), (mp_obj_t)&modaudio_play_stop_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_play_pause), (mp_obj_t)&modaudio_play_pause_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_play_resume), (mp_obj_t)&modaudio_play_resume_obj },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_audio_globals, mp_module_audio_globals_table);


const mp_obj_module_t audio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_audio_globals,
};
