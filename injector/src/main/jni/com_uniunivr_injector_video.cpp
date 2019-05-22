//
// Created by pedro on 9/11/16.
//

#include "com_uniunivr_injector_video.h"
#include <iostream>
#include <stdlib.h>
#include "spatialmedia/parser.h"
#include "spatialmedia/metadata_utils.h"


using namespace std;
using namespace SpatialMedia;


#define TRUE  1
#define FALSE 0

JNIEXPORT jboolean JNICALL Java_com_uniunivr_injector_video_VideoInjector_injectVideo
        (JNIEnv *env, jobject, jstring fileIn, jstring fileOut) {
    Parser parser;
    const char *nativeStringIn = env->GetStringUTFChars(fileIn, 0);
    const char *nativeStringOut = env->GetStringUTFChars(fileOut, 0);
    parser.m_strInFile = nativeStringIn;
    parser.m_strOutFile = nativeStringOut;
    parser.getInFile();
    parser.getOutFile();
    if (parser.getInFile() == "") {
        LOGI("Please provide an input file.");
        return FALSE;
    }
    Utils utils;
    if (parser.getInject()) {
        if (parser.getOutFile() == "") {
            LOGI("Injecting metadata requires both input and output file.");
            return FALSE;
        }
        Metadata md;
        std::string &strVideoXML = utils.generate_spherical_xml(parser.getStereoMode(),
                                                                parser.getCrop());
        md.setVideoXML(strVideoXML);
        LOGI( "xml created success");
        if (parser.getSpatialAudio()) {
            md.setAudio(&g_DefAudioMetadata);
            LOGI( "metadata audio setted success");
        }
        if (strVideoXML.length() > 1) {
            utils.inject_metadata(parser.getInFile(), parser.getOutFile(), &md);
            LOGI( "metadata injected success");
        }
        else
            LOGI("Failed to generate metadata.");
    }
    return TRUE;

}