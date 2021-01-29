/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
process pictures of the moon.

some things we've learned about libraw and/or this camera.
open_file(filename)
unpack()
raw2image();
imgdata.sizes.width
imgdata.sizes.height
idx = x + y * width
imgdata.image[idx][c]
where c is the color component index is:
0: red
1: green 1
2: blue
3: green 2
note: ONE of those 4 pixels will have a value.
the other THREE will be zero.
pretty sure the bayer pattern is:
     x     x+1
y  : R/0   G1/1
y+1: G2/3  B/2
where the /# is the above color component index as per above.
one assumes one is expected to interpolate rgb values for everywhere else.
the rgb component values range from about 2k to 14k.
it's not clear how to get these numbers from the imgdata.
each r g1 b g2 component has its own slightly different black value.
r and b seem to correlate very tightly.
but g1 g2 seem to be out to lunch.
they seem to need need to be scaled (after subracting black) by about 1.9.

re interpolation...
currently we interopolate rgbg values for every pixel.
using a 1331 filter.
there are probably other methods.

note: there seems to be a black band top and left of some images.
perhaps there's padding in the image data.

note: there seem to be more r&b pixels than g pixels.
strange. why?

note: the base number seems to be consistent across shots.
the factor number is not.
some shots the g1 and g2 histograms are somewhat different.
it almost looks like the greens need to be offset.
maybe by the difference in the averages excluding the near blacks?

note: where are the pixels with base values?
are they near the borders?
like maybe not illuminated?
are they maybe literally camera noise?
**/

/**
the moon is about 724 pixels wide.
it's area is about 400k pixels.
the image is 6000*4000
so the moon is about 1.7% of the pixels.
count pixels from brightest to get to 1%.
above that threshold we're definitely in the moon.
90% of the pixels are black sky pixels.
count pixels from the darkest to get to 90%.
below that threshold we're definitely in the black.
average the two thresholds for moon/not-moon pixels.

or...
use joint index.
where X% of the pixels consume 1-X% of the range.
**/

#include <libraw/libraw.h>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <common/png.h>

#include <vector>
#include <sstream>

namespace {

const char *kInputFilename = "/home/timmer/Pictures/2020-05-12/moon/IMG_0393.CR2";
const char *kOutputFilename = "moon.png";

//const char *kInputFilename = "/home/timmer/Pictures/2020-07-11/IMG_0480.CR2";
//const char *kOutputFilename = "comet1.png";

//const char *kInputFilename = "/home/timmer/Pictures/2020-07-11/IMG_0481.CR2";
//const char *kOutputFilename = "comet2.png";

/**
bins in histogram.
**/
static const int kBins = 512;

/**
these hard coded numbers numbers come from pasting the histograms into a spreadsheet.
and tweaking until the rbgg graphs more/less line up.
how do we compute them automagically?
**/
/**
largest possible value recorded by camera.
**/
static const int kRange = 16384;
/**
"zero" for scaling.
**/
static const int kBase = 64.0 * kRange / kBins; // =2048
/**
kFactor scales greens by this amount.
preserve "zero".
**/
static const float kFactor = 1.9;
/**
maps to 0.
**/
static const int kZero = kBase - 3*32;
/**
maps to 255.
**/
static const int kSaturation = 441.0 * kRange / kBins;

unsigned short *get_pixel(
    LibRaw& raw_image,
    int x,
    int y,
    int cmp
) {
    int wd = raw_image.imgdata.sizes.width;
    int ht = raw_image.imgdata.sizes.height;
    x = std::max(x, 0);
    y = std::max(y, 0);
    x = std::min(x, wd-1);
    y = std::min(y, ht-1);
    int idx = x + wd*y;
    auto pixel = &raw_image.imgdata.image[idx][cmp];
    return pixel;
}

void interpolate1331(
    LibRaw& raw_image,
    int x,
    int y,
    int dx,
    int dy,
    int cmp
) {
    int p0 = *get_pixel(raw_image, x-3*dx, y-3*dy, cmp);
    int p1 = *get_pixel(raw_image, x-1*dx, y-1*dy, cmp);
    int p2 = *get_pixel(raw_image, x+1*dx, y+1*dy, cmp);
    int p3 = *get_pixel(raw_image, x+3*dx, y+3*dy, cmp);
    auto pixel = get_pixel(raw_image, x, y, cmp);
    *pixel = (p0 + 3*p1 + 3*p2 + p3 - 8*kZero)/8 + kZero;
}

int scale_to_255(
    int c0,
    int c1,
    float factor
) {
    c0 = (c0 - kBase) * factor + kBase;
    c1 = (c1 - kBase) * factor + kBase;
    int c = c0 - kZero + c1 - kZero;
    c *= 255;
    c /= 2 * (kSaturation - kZero);
    c = std::max(0, std::min(c, 255));
    return c;
}

std::string quote_or_null(
    const char *str
) {
    if (str == nullptr) {
        return "(nullptr)";
    }
    std::string s("\"");
    s += str;
    s += "\"";
    return s;
}

void dump(const LibRaw &raw_image) {
    auto& imgdata = raw_image.imgdata;
    LOG("dumping raw_image:");
    LOG("imgdata.image[0]="<<(void*)imgdata.image[0]);
    LOG("imgdata.image[1]="<<(void*)imgdata.image[0]);
    LOG("imgdata.image[2]="<<(void*)imgdata.image[0]);
    LOG("imgdata.image[3]="<<(void*)imgdata.image[0]);
    LOG("imgdata.sizes.raw_height="<<imgdata.sizes.raw_height);
    LOG("imgdata.sizes.raw_width="<<imgdata.sizes.raw_width);
    LOG("imgdata.sizes.height="<<imgdata.sizes.height);
    LOG("imgdata.sizes.width="<<imgdata.sizes.width);
    LOG("imgdata.sizes.top_margin="<<imgdata.sizes.top_margin);
    LOG("imgdata.sizes.left_margin="<<imgdata.sizes.left_margin);
    LOG("imgdata.sizes.iheight="<<imgdata.sizes.iheight);
    LOG("imgdata.sizes.iwidth="<<imgdata.sizes.iwidth);
    LOG("imgdata.sizes.raw_pitch="<<imgdata.sizes.raw_pitch);
    LOG("imgdata.sizes.pixel_aspect="<<imgdata.sizes.pixel_aspect);
    LOG("imgdata.sizes.flip="<<imgdata.sizes.flip);
    std::stringstream ss;
    for (int i = 0; i < 8; ++i) {
        for (int k = 0; k < 4; ++k) {
            ss<<imgdata.sizes.mask[i][k]<<" ";
        }
    }
    LOG("imgdata.sizes.mask=[ "<<ss.str()<<"]");
    LOG("imgdata.sizes.raw_crop.cleft="<<imgdata.sizes.raw_crop.cleft);
    LOG("imgdata.sizes.raw_crop.ctop="<<imgdata.sizes.raw_crop.ctop);
    LOG("imgdata.sizes.raw_crop.cwidth="<<imgdata.sizes.raw_crop.cwidth);
    LOG("imgdata.sizes.raw_crop.cheight="<<imgdata.sizes.raw_crop.cheight);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<(int)imgdata.idata.guard[i]<<" ";
    }
    LOG("imgdata.idata.guard=[ "<<ss.str()<<"]");
    LOG("imgdata.idata.make="<<quote_or_null(imgdata.idata.make));
    LOG("imgdata.idata.model="<<quote_or_null(imgdata.idata.model));
    LOG("imgdata.idata.software="<<quote_or_null(imgdata.idata.software));
    LOG("imgdata.idata.raw_count="<<imgdata.idata.raw_count);
    LOG("imgdata.idata.dng_version="<<imgdata.idata.dng_version);
    LOG("imgdata.idata.is_foveon="<<imgdata.idata.is_foveon);
    LOG("imgdata.idata.colors="<<imgdata.idata.colors);
    LOG("imgdata.idata.filters="<<imgdata.idata.filters);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 6; ++i) {
        for (int k = 0; k < 6; ++k) {
            ss<<(int)imgdata.idata.xtrans[i][k]<<" ";
        }
    }
    LOG("imgdata.idata.xtrans=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 6; ++i) {
        for (int k = 0; k < 6; ++k) {
            ss<<(int)imgdata.idata.xtrans_abs[i][k]<<" ";
        }
    }
    LOG("imgdata.idata.xtrans_abs=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.idata.cdesc[i];
    }
    LOG("imgdata.idata.cdesc='"<<ss.str()<<"'");
    LOG("imgdata.idata.xmplen="<<imgdata.idata.xmplen);
    LOG("imgdata.idata.xmpdata="<<(void*)imgdata.idata.xmpdata);
    LOG("imgdata.lens.MinFocal="<<imgdata.lens.MinFocal);
    LOG("imgdata.lens.MaxFocal="<<imgdata.lens.MaxFocal);
    LOG("imgdata.lens.MaxAp4MinFocal="<<imgdata.lens.MaxAp4MinFocal);
    LOG("imgdata.lens.MaxAp4MaxFocal="<<imgdata.lens.MaxAp4MaxFocal);
    LOG("imgdata.lens.EXIF_MaxAp="<<imgdata.lens.EXIF_MaxAp);
    LOG("imgdata.lens.LensMake="<<quote_or_null(imgdata.lens.LensMake));
    LOG("imgdata.lens.Lens="<<quote_or_null(imgdata.lens.Lens));
    LOG("imgdata.lens.LensSerial="<<quote_or_null(imgdata.lens.LensSerial));
    LOG("imgdata.lens.InternalLensSerial="<<quote_or_null(imgdata.lens.InternalLensSerial));
    LOG("imgdata.lens.FocalLengthIn35mmFormat="<<imgdata.lens.FocalLengthIn35mmFormat);
    LOG("imgdata.lens.nikon.NikonEffectiveMaxAp="<<imgdata.lens.nikon.NikonEffectiveMaxAp);
    LOG("imgdata.lens.nikon.NikonLensIDNumber="<<(int)imgdata.lens.nikon.NikonLensIDNumber);
    LOG("imgdata.lens.nikon.NikonLensFStops="<<(int)imgdata.lens.nikon.NikonLensFStops);
    LOG("imgdata.lens.nikon.NikonMCUVersion="<<(int)imgdata.lens.nikon.NikonMCUVersion);
    LOG("imgdata.lens.nikon.NikonLensType="<<(int)imgdata.lens.nikon.NikonLensType);
    LOG("imgdata.lens.dng.MinFocal="<<(int)imgdata.lens.dng.MinFocal);
    LOG("imgdata.lens.dng.MaxFocal="<<(int)imgdata.lens.dng.MaxFocal);
    LOG("imgdata.lens.dng.MaxAp4MinFocal="<<(int)imgdata.lens.dng.MaxAp4MinFocal);
    LOG("imgdata.lens.dng.MaxAp4MaxFocal="<<(int)imgdata.lens.dng.MaxAp4MaxFocal);
    LOG("imgdata.lens.makernotes=not dumped");
    /*
    LOG("imgdata.lens.makernotes.LensID="<<imgdata.lens.makernotes.LensID);
    LOG("imgdata.lens.makernotes.Lens="<<quote_or_null(imgdata.lens.makernotes.Lens));
    LOG("imgdata.lens.makernotes.LensFormat="<<imgdata.lens.makernotes.LensFormat);
    LOG("imgdata.lens.makernotes.LensMount="<<imgdata.lens.makernotes.LensMount);
    LOG("imgdata.lens.makernotes.CamID="<<imgdata.lens.makernotes.CamID);
    LOG("imgdata.lens.makernotes.CameraFormat="<<imgdata.lens.makernotes.CameraFormat);
    LOG("imgdata.lens.makernotes.CameraMount="<<imgdata.lens.makernotes.CameraMount);
    LOG("imgdata.lens.makernotes.body="<<quote_or_null(imgdata.lens.makernotes.body));
    LOG("imgdata.lens.makernotes.FocalType="<<imgdata.lens.makernotes.FocalType);
    LOG("imgdata.lens.makernotes.LensFeatures_pre="<<quote_or_null(imgdata.lens.makernotes.LensFeatures_pre));
    LOG("imgdata.lens.makernotes.LensFeatures_suf="<<quote_or_null(imgdata.lens.makernotes.LensFeatures_suf));
    LOG("imgdata.lens.makernotes.MinFocal="<<imgdata.lens.makernotes.MinFocal);
    LOG("imgdata.lens.makernotes.MaxFocal="<<imgdata.lens.makernotes.MaxFocal);
    LOG("imgdata.lens.makernotes.MaxAp4MinFocal="<<imgdata.lens.makernotes.MaxAp4MinFocal);
    LOG("imgdata.lens.makernotes.MaxAp4MaxFocal="<<imgdata.lens.makernotes.MaxAp4MaxFocal);
    LOG("imgdata.lens.makernotes.MinAp4MinFocal="<<imgdata.lens.makernotes.MinAp4MinFocal);
    LOG("imgdata.lens.makernotes.MinAp4MaxFocal="<<imgdata.lens.makernotes.MinAp4MaxFocal);
    LOG("imgdata.lens.makernotes.MaxAp="<<imgdata.lens.makernotes.MaxAp);
    LOG("imgdata.lens.makernotes.MinAp="<<imgdata.lens.makernotes.MinAp);
    LOG("imgdata.lens.makernotes.CurFocal="<<imgdata.lens.makernotes.CurFocal);
    LOG("imgdata.lens.makernotes.CurAp="<<imgdata.lens.makernotes.CurAp);
    LOG("imgdata.lens.makernotes.MaxAp4CurFocal="<<imgdata.lens.makernotes.MaxAp4CurFocal);
    LOG("imgdata.lens.makernotes.MinAp4CurFocal="<<imgdata.lens.makernotes.MinAp4CurFocal);
    LOG("imgdata.lens.makernotes.MinFocusDistance="<<imgdata.lens.makernotes.MinFocusDistance);
    LOG("imgdata.lens.makernotes.FocusRangeIndex="<<imgdata.lens.makernotes.FocusRangeIndex);
    LOG("imgdata.lens.makernotes.LensFStops="<<imgdata.lens.makernotes.LensFStops);
    LOG("imgdata.lens.makernotes.TeleconverterID="<<imgdata.lens.makernotes.TeleconverterID);
    LOG("imgdata.lens.makernotes.Teleconverter="<<quote_or_null(imgdata.lens.makernotes.Teleconverter));
    LOG("imgdata.lens.makernotes.AdapterID="<<imgdata.lens.makernotes.AdapterID);
    LOG("imgdata.lens.makernotes.Adapter="<<quote_or_null(imgdata.lens.makernotes.Adapter));
    LOG("imgdata.lens.makernotes.AttachmentID="<<imgdata.lens.makernotes.AttachmentID);
    LOG("imgdata.lens.makernotes.Attachment="<<quote_or_null(imgdata.lens.makernotes.Attachment));
    LOG("imgdata.lens.makernotes.CanonFocalUnits="<<imgdata.lens.makernotes.CanonFocalUnits);
    LOG("imgdata.lens.makernotes.FocalLengthIn35mmFormat="<<imgdata.lens.makernotes.FocalLengthIn35mmFormat);
    */
    LOG("imgdata.makernotes=not dumped");
    /*
    libraw_makernotes_t makernotes;
    */
    LOG("imgdata.shootinginfo.DriveMode="<<imgdata.shootinginfo.DriveMode);
    LOG("imgdata.shootinginfo.FocusMode="<<imgdata.shootinginfo.FocusMode);
    LOG("imgdata.shootinginfo.MeteringMode="<<imgdata.shootinginfo.MeteringMode);
    LOG("imgdata.shootinginfo.AFPoint="<<imgdata.shootinginfo.AFPoint);
    LOG("imgdata.shootinginfo.ExposureMode="<<imgdata.shootinginfo.ExposureMode);
    LOG("imgdata.shootinginfo.ImageStabilization="<<imgdata.shootinginfo.ImageStabilization);
    LOG("imgdata.shootinginfo.BodySerial="<<quote_or_null(imgdata.shootinginfo.BodySerial));
    LOG("imgdata.shootinginfo.InternalBodySerial="<<quote_or_null(imgdata.shootinginfo.InternalBodySerial));
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.params.greybox[i]<<" ";
    }
    LOG("imgdata.params.greybox=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.params.cropbox[i]<<" ";
    }
    LOG("imgdata.params.cropbox=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.params.aber[i]<<" ";
    }
    LOG("imgdata.params.aber=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 6; ++i) {
        ss<<imgdata.params.gamm[i]<<" ";
    }
    LOG("imgdata.params.gamm=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.params.user_mul[i]<<" ";
    }
    LOG("imgdata.params.user_mul=[ "<<ss.str()<<"]");
    LOG("imgdata.params.shot_select="<<imgdata.params.shot_select);
    LOG("imgdata.params.bright="<<imgdata.params.bright);
    LOG("imgdata.params.threshold="<<imgdata.params.threshold);
    LOG("imgdata.params.half_size="<<imgdata.params.half_size);
    LOG("imgdata.params.four_color_rgb="<<imgdata.params.four_color_rgb);
    LOG("imgdata.params.highlight="<<imgdata.params.highlight);
    LOG("imgdata.params.use_auto_wb="<<imgdata.params.use_auto_wb);
    LOG("imgdata.params.use_camera_wb="<<imgdata.params.use_camera_wb);
    LOG("imgdata.params.use_camera_matrix="<<imgdata.params.use_camera_matrix);
    LOG("imgdata.params.output_color="<<imgdata.params.output_color);
    LOG("imgdata.params.output_profile="<<quote_or_null(imgdata.params.output_profile));
    LOG("imgdata.params.bad_pixels="<<quote_or_null(imgdata.params.bad_pixels));
    LOG("imgdata.params.dark_frame="<<quote_or_null(imgdata.params.dark_frame));
    LOG("imgdata.params.output_bps="<<imgdata.params.output_bps);
    LOG("imgdata.params.output_tiff="<<imgdata.params.output_tiff);
    LOG("imgdata.params.user_flip="<<imgdata.params.user_flip);
    LOG("imgdata.params.user_qual="<<imgdata.params.user_qual);
    LOG("imgdata.params.user_black="<<imgdata.params.user_black);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.params.user_cblack[i]<<" ";
    }
    LOG("imgdata.params.user_cblack=[ "<<ss.str()<<"]");
    LOG("imgdata.params.user_sat="<<imgdata.params.user_sat);
    LOG("imgdata.params.med_passes="<<imgdata.params.med_passes);
    LOG("imgdata.params.auto_bright_thr="<<imgdata.params.auto_bright_thr);
    LOG("imgdata.params.adjust_maximum_thr="<<imgdata.params.adjust_maximum_thr);
    LOG("imgdata.params.no_auto_bright="<<imgdata.params.no_auto_bright);
    LOG("imgdata.params.use_fuji_rotate="<<imgdata.params.use_fuji_rotate);
    LOG("imgdata.params.green_matching="<<imgdata.params.green_matching);
    LOG("imgdata.params.dcb_iterations="<<imgdata.params.dcb_iterations);
    LOG("imgdata.params.dcb_enhance_fl="<<imgdata.params.dcb_enhance_fl);
    LOG("imgdata.params.fbdd_noiserd="<<imgdata.params.fbdd_noiserd);
    LOG("imgdata.params.exp_correc="<<imgdata.params.exp_correc);
    LOG("imgdata.params.exp_shift="<<imgdata.params.exp_shift);
    LOG("imgdata.params.exp_preser="<<imgdata.params.exp_preser);
    LOG("imgdata.params.use_rawspeed="<<imgdata.params.use_rawspeed);
    LOG("imgdata.params.use_dngsdk="<<imgdata.params.use_dngsdk);
    LOG("imgdata.params.no_auto_scale="<<imgdata.params.no_auto_scale);
    LOG("imgdata.params.no_interpolation="<<imgdata.params.no_interpolation);
    LOG("imgdata.params.raw_processing_options="<<imgdata.params.raw_processing_options);
    LOG("imgdata.params.sony_arw2_posterization_thr="<<imgdata.params.sony_arw2_posterization_thr);
    LOG("imgdata.params.coolscan_nef_gamma="<<imgdata.params.coolscan_nef_gamma);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 5; ++i) {
        ss<<(int)imgdata.params.p4shot_order[i]<<" ";
    }
    LOG("imgdata.params.p4shot_order=[ "<<ss.str()<<"]");
    LOG("imgdata.params.custom_camera_strings="<<(void*)imgdata.params.custom_camera_strings);
    LOG("imgdata.progress_flags="<<imgdata.progress_flags);
    LOG("imgdata.process_warnings="<<imgdata.process_warnings);
    LOG("imgdata.color.curve=[not available]");
    LOG("imgdata.color.cblack=[not available]");
    LOG("imgdata.color.black="<<imgdata.color.black);
    LOG("imgdata.color.data_maximum="<<imgdata.color.data_maximum);
    LOG("imgdata.color.maximum="<<imgdata.color.maximum);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 5; ++i) {
        ss<<imgdata.color.linear_max[i]<<" ";
    }
    LOG("imgdata.color.linear_max=[ "<<ss.str()<<"]");
    LOG("imgdata.color.fmaximum="<<imgdata.color.fmaximum);
    LOG("imgdata.color.fnorm="<<imgdata.color.fnorm);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 8; ++i) {
        for (int k = 0; k < 8; ++k) {
            ss<<imgdata.color.white[i][k]<<" ";
        }
    }
    LOG("imgdata.color.white=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.color.cam_mul[i]<<" ";
    }
    LOG("imgdata.color.cam_mul=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.color.pre_mul[i]<<" ";
    }
    LOG("imgdata.color.pre_mul=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 4; ++k) {
            ss<<imgdata.color.cmatrix[i][k]<<" ";
        }
    }
    LOG("imgdata.color.cmatrix=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 4; ++k) {
            ss<<imgdata.color.ccm[i][k]<<" ";
        }
    }
    LOG("imgdata.color.ccm=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 4; ++k) {
            ss<<imgdata.color.rgb_cam[i][k]<<" ";
        }
    }
    LOG("imgdata.color.rgb_cam=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        for (int k = 0; k < 3; ++k) {
            ss<<imgdata.color.cam_xyz[i][k]<<" ";
        }
    }
    LOG("imgdata.color.cam_xyz=[ "<<ss.str()<<"]");
    LOG("imgdata.color.phase_one_data.format="<<imgdata.color.phase_one_data.format);
    LOG("imgdata.color.phase_one_data.key_off="<<imgdata.color.phase_one_data.key_off);
    LOG("imgdata.color.phase_one_data.tag_21a="<<imgdata.color.phase_one_data.tag_21a);
    LOG("imgdata.color.phase_one_data.t_black="<<imgdata.color.phase_one_data.t_black);
    LOG("imgdata.color.phase_one_data.split_col="<<imgdata.color.phase_one_data.split_col);
    LOG("imgdata.color.phase_one_data.black_col="<<imgdata.color.phase_one_data.black_col);
    LOG("imgdata.color.phase_one_data.split_row="<<imgdata.color.phase_one_data.split_row);
    LOG("imgdata.color.phase_one_data.black_row="<<imgdata.color.phase_one_data.black_row);
    LOG("imgdata.color.phase_one_data.tag_210="<<imgdata.color.phase_one_data.tag_210);
    LOG("imgdata.color.flash_used="<<imgdata.color.flash_used);
    LOG("imgdata.color.canon_ev="<<imgdata.color.canon_ev);
    LOG("imgdata.color.model2="<<quote_or_null(imgdata.color.model2));
    LOG("imgdata.color.UniqueCameraModel="<<quote_or_null(imgdata.color.UniqueCameraModel));
    LOG("imgdata.color.LocalizedCameraModel="<<quote_or_null(imgdata.color.LocalizedCameraModel));
    LOG("imgdata.color.profile="<<(void*)imgdata.color.profile);
    LOG("imgdata.color.profile_length="<<imgdata.color.profile_length);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 8; ++i) {
        ss<<imgdata.color.black_stat[i]<<" ";
    }
    LOG("imgdata.color.black_stat=[ "<<ss.str()<<"]");
    for (int n = 0; n < 2; ++n) {
        auto& dng_color = imgdata.color.dng_color[n];
        LOG("imgdata.color.dng_color["<<n<<"].parsedfields="<<dng_color.parsedfields);
        LOG("imgdata.color.dng_color["<<n<<"].illuminant="<<dng_color.illuminant);
        ss.clear();
        ss.str(std::string());
        for (int i = 0; i < 4; ++i) {
            for (int k = 0; k < 4; ++k) {
                ss<<dng_color.calibration[i][k]<<" ";
            }
        }
        LOG("imgdata.color.dng_color["<<n<<"].calibration=[ "<<ss.str()<<"]");
        ss.clear();
        ss.str(std::string());
        for (int i = 0; i < 4; ++i) {
            for (int k = 0; k < 3; ++k) {
                ss<<dng_color.colormatrix[i][k]<<" ";
            }
        }
        LOG("imgdata.color.dng_color["<<n<<"].colormatrix=[ "<<ss.str()<<"]");
        ss.clear();
        ss.str(std::string());
        for (int i = 0; i < 3; ++i) {
            for (int k = 0; k < 4; ++k) {
                ss<<dng_color.forwardmatrix[i][k]<<" ";
            }
        }
        LOG("imgdata.color.dng_color["<<n<<"].forwardmatrix=[ "<<ss.str()<<"]");
    }
    LOG("imgdata.color.dng_levels.parsedfields="<<imgdata.color.dng_levels.parsedfields);
    LOG("imgdata.color.dng_levels.dng_cblack=not dumped");
    LOG("imgdata.color.dng_levels.dng_black="<<imgdata.color.dng_levels.dng_black);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.color.dng_levels.dng_whitelevel[i]<<" ";
    }
    LOG("imgdata.color.dng_levels.dng_whitelevel=[ "<<ss.str()<<"]");
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.color.dng_levels.default_crop[i]<<" ";
    }
    LOG("imgdata.color.dng_levels.default_crop=[ "<<ss.str()<<"]");
    LOG("imgdata.color.dng_levels.preview_colorspace="<<imgdata.color.dng_levels.preview_colorspace);
    ss.clear();
    ss.str(std::string());
    for (int i = 0; i < 4; ++i) {
        ss<<imgdata.color.dng_levels.analogbalance[i]<<" ";
    }
    LOG("imgdata.color.dng_levels.analogbalance=[ "<<ss.str()<<"]");
    LOG("imgdata.color.baseline_exposure="<<imgdata.color.baseline_exposure);
    LOG("imgdata.color.WB_Coeffs=not dumped");
    LOG("imgdata.color.WBCT_Coeffs=not dumped");
    for (int n = 0; n < 2; ++n) {
        auto& P1_color = imgdata.color.P1_color[n];
        ss.clear();
        ss.str(std::string());
        for (int i = 0; i < 9; ++i) {
            ss<<P1_color.romm_cam[i]<<" ";
        }
        LOG("imgdata.color.P1_color["<<n<<"].calibration=[ "<<ss.str()<<"]");
    }
    /*
    libraw_colordata_t color;
    ushort curve[0x10000];
    unsigned cblack[4102];

    libraw_imgother_t other;
    libraw_thumbnail_t thumbnail;
    libraw_rawdata_t rawdata;
    */
    LOG("imgdata.parent_class="<<(void*)imgdata.parent_class);
}

}

int main(
    int argc, char *argv[]
) noexcept {
    const char *in_filename = kInputFilename;
    const char *out_filename = kOutputFilename;
    if (argc > 1) {
        in_filename = argv[1];
    }
    if (argc > 2) {
        out_filename = argv[2];
    }

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG("Hello, World!");

    /** load image **/
    LibRaw raw_image;
    raw_image.open_file(in_filename);
    dump(raw_image);
    return 0;
    int wd = raw_image.imgdata.sizes.width;
    int ht = raw_image.imgdata.sizes.height;
    LOG("wd="<<wd);
    LOG("ht="<<ht);
    if (wd <= 0 || ht <= 0) {
        LOG("File not opened: "<<in_filename);
        return 1;
    }
    raw_image.unpack();
    raw_image.raw2image();

    /** analyze image **/
    int minr = 99999;
    int maxr = 0;
    int minb = 99999;
    int maxb = 0;
    int ming1 = 99999;
    int maxg1 = 0;
    int ming2 = 99999;
    int maxg2 = 0;
    std::int64_t sumr = 0;
    std::int64_t sumb = 0;
    std::int64_t sumg1 = 0;
    std::int64_t sumg2 = 0;
    int npixelsr = 0;
    int npixelsb = 0;
    int npixelsg1 = 0;
    int npixelsg2 = 0;
    std::vector<int> histr(kBins,0);
    std::vector<int> histb(kBins,0);
    std::vector<int> histg1(kBins,0);
    std::vector<int> histg2(kBins,0);
    std::vector<int> histg3(kBins,0);
    std::vector<int> histg4(kBins,0);

    for (int y = 0; y < ht; ++y) {
        for (int x = 0; x < wd; ++x) {
            int idx = x + y*wd;
            auto pixel = raw_image.imgdata.image[idx];
            //LOG(y<<": r="<<pixel[0]<<" g1="<<pixel[1]<<" b="<<pixel[2]<<" g2="<<pixel[3]);
            int r = pixel[0];
            int b = pixel[2];
            int g1 = pixel[1];
            int g2 = pixel[3];
            int g3 = 0;
            int g4 = 0;
            if (r > 0) {
                minr = std::min(minr, r);
                maxr = std::max(maxr, r);
            }
            if (b > 0) {
                minb = std::min(minb, b);
                maxb = std::max(maxb, b);
            }
            if (g1 > 0) {
                ming1 = std::min(ming1, g1);
                maxg1 = std::max(maxg1, g1);
                g3 = (g1 - kBase) * kFactor + kBase;
            }
            if (g2 > 0) {
                ming2 = std::min(ming2, g2);
                maxg2 = std::max(maxg2, g2);
                g4 = (g2 - kBase) * kFactor + kBase;
            }
            sumr += r;
            sumb += b;
            sumg1 += g1;
            sumg2 += g2;
            int rx = r * kBins/kRange;
            int bx = b * kBins/kRange;
            int g1x = g1 * kBins/kRange;
            int g2x = g2 * kBins/kRange;
            int g3x = g3 * kBins/kRange;
            int g4x = g4 * kBins/kRange;
            rx = std::max(std::min(rx, kBins-1), 0);
            bx = std::max(std::min(bx, kBins-1), 0);
            g1x = std::max(std::min(g1x, kBins-1), 0);
            g2x = std::max(std::min(g2x, kBins-1), 0);
            g3x = std::max(std::min(g3x, kBins-1), 0);
            g4x = std::max(std::min(g4x, kBins-1), 0);
            if (r > 0) {
                histr[rx] += 1;
                ++npixelsr;
            }
            if (b > 0) {
                histb[bx] += 1;
                ++npixelsb;
            }
            if (g1 > 0) {
                histg1[g1x] += 1;
                ++npixelsg1;
            }
            if (g2 > 0) {
                histg2[g2x] += 1;
                ++npixelsg2;
            }
            if (g3 > 0) {
                histg3[g3x] += 1;
            }
            if (g4 > 0) {
                histg4[g4x] += 1;
            }
        }
    }
    int avgr = sumr / npixelsr;
    int avgb = sumb / npixelsb;
    int avgg1 = sumg1 / npixelsg1;
    int avgg2 = sumg2 / npixelsg2;
    LOG("npixels r/b/g1/g2="<<npixelsr<<"/"<<npixelsb<<"/"<<npixelsg1<<"/"<<npixelsg2);
    LOG("min/avg/max r="<<minr<<"/"<<avgr<<"/"<<maxr);
    LOG("min/avg/max b="<<minb<<"/"<<avgb<<"/"<<maxb);
    LOG("min/avg/max g1="<<ming1<<"/"<<avgg1<<"/"<<maxg1);
    LOG("min/avg/max g2="<<ming2<<"/"<<avgg2<<"/"<<maxg2);
    std::cout<<"histogram r:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histr[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram b:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histb[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g1:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg1[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g2:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg2[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g3:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg3[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g4:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg4[i];
    }
    std::cout<<std::endl;

#if 1
    int wd2 = wd/2;
    int ht2 = ht/2;

    /** interpolate by y **/
    for (int y = 0; y < ht2; ++y) {
        for (int x = 0; x < wd2; ++x) {
            interpolate1331(raw_image, 2*x, 2*y+1, 0, 1, 0);
            interpolate1331(raw_image, 2*x+1, 2*y+1, 0, 1, 1);
            interpolate1331(raw_image, 2*x+1, 2*y, 0, 1, 2);
            interpolate1331(raw_image, 2*x, 2*y, 0, 1, 3);
        }
    }
    /** interpolate by x **/
    for (int y = 0; y < ht2; ++y) {
        for (int x = 0; x < wd2; ++x) {
            interpolate1331(raw_image, 2*x+1, 2*y, 1, 0, 0);
            interpolate1331(raw_image, 2*x+1, 2*y+1, 1, 0, 0);
            interpolate1331(raw_image, 2*x, 2*y, 1, 0, 1);
            interpolate1331(raw_image, 2*x, 2*y+1, 1, 0, 1);
            interpolate1331(raw_image, 2*x, 2*y, 1, 0, 2);
            interpolate1331(raw_image, 2*x, 2*y+1, 1, 0, 2);
            interpolate1331(raw_image, 2*x+1, 2*y, 1, 0, 3);
            interpolate1331(raw_image, 2*x+1, 2*y+1, 1, 0, 3);
        }
    }

    Png png;
    png.init(wd, ht);
    for (int y = 0; y < ht; ++y) {
        for (int x = 0; x < wd; ++x) {
            int idx = x + y*wd;
            auto pixel = raw_image.imgdata.image[idx];
            int r0 = pixel[0];
            int b0 = pixel[2];
            int g1 = pixel[1];
            int g2 = pixel[3];
            int r = scale_to_255(r0, r0, 1.0);
            int b = scale_to_255(b0, b0, 1.0);
            int g = scale_to_255(g1, g2, kFactor);
            idx = x * 3 + y * png.stride_;
            png.data_[idx] = r;
            png.data_[idx+1] = g;
            png.data_[idx+2] = b;
        }
    }
    png.write(out_filename);
#else
    (void) out_filename;
#endif

    raw_image.recycle();

    LOG("Goodbye, World!");

    return 0;
}
