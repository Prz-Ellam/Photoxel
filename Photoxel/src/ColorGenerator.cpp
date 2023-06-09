#include "ColorGenerator.h"

//dlib::rgb_pixel GetBasicColor(int value) {
//    int r, g, b;
//
//    if (value == 0) {
//        r = 255;
//        g = 0;
//        b = 0;
//    }
//    else {
//        std::hash<int> hash;
//        r = hash(value) % 256;
//        g = hash(value + 1) % 256;
//        b = hash(value + 2) % 256;
//    }
//
//    return dlib::rgb_pixel(r, g, b);
//}

cv::Scalar GetBasicColor(int value) {
    int r, g, b;

    if (value == 0) {
        r = 255;
        g = 0;
        b = 0;
    }
    else {
        std::hash<int> hash;
        r = hash(value) % 256;
        g = hash(value + 1) % 256;
        b = hash(value + 2) % 256;
    }

    return cv::Scalar(r, g, b);
}
