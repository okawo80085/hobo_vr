#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

extern "C"
{
    #include <libavcodec/avcodec.h>
}

TEST_CASE( "bob", "[f]" ) {
    //REQUIRE(avcodec_find_encoder_by_name("mpeg4")!=nullptr);
    //REQUIRE(avcodec_find_encoder_by_name("libvp8")!=nullptr);
    //REQUIRE(avcodec_find_encoder_by_name("libvpx")!=nullptr);
    AVCodec * a = nullptr;
    do{
        //INFO(a);
        a = av_codec_next(a);
        //INFO(a);
        INFO("hi\n");
        
        /*if(a->type==AVMEDIA_TYPE_VIDEO){
            if (av_codec_is_encoder(a)){
                auto encoder = "video encoder: "+ std::string(a->name);
                WARN(encoder);
            }else if(av_codec_is_decoder(a)){
                auto decoder = "video decoder: "+ std::string(a->name);
                WARN(decoder);
            }
        }*/
        if(a->type==AVMEDIA_TYPE_AUDIO){
            if (av_codec_is_encoder(a)){
                auto encoder = "audio encoder: "+ std::string(a->name);
                WARN(encoder);
            }else if(av_codec_is_decoder(a)){
                auto decoder = "audio decoder: "+ std::string(a->name);
                WARN(decoder);
            }
        }
        REQUIRE(a!=nullptr);
    }while(a->next!=nullptr);

}