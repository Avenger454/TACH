#include "../ui.h"
#include <math.h>

/* Reference artwork is an unmodified RGB565 conversion of the supplied mockup.
 * The original sample digits are covered by an opaque LVGL panel. */
extern const uint8_t face_start[] asm("_binary_tach_face_rgb565_start");
LV_FONT_DECLARE(tach_digits);
static const lv_img_dsc_t face = {
    .header = {.cf = LV_IMG_CF_TRUE_COLOR, .w = 512, .h = 512},
    .data_size = 512 * 512 * 2, .data = face_start
};
lv_obj_t *ui_Screen1, *ui_rpm_bg, *ui_rpm_arc;
lv_obj_t *ui_label_mph_value, *ui_label_mph, *ui_label_gear;
lv_obj_t *ui_label_gear_value, *ui_label_odometer_value;
static lv_obj_t *rpm_label;
static int last_rpm = -1;
static int last_gear = -99;
static const lv_point_t gear_outline[] = {
    {0, 85}, {36, 0}, {196, 0}, {232, 85},
    {224, 99}, {8, 99}, {0, 85}
};
/* LVGL keeps this array by reference. */
static lv_point_t pointer_points[2];
static lv_point_t last_screen_points[2];
static bool pointer_position_valid;


/* Static accents beside the artwork's RPM title. Layered strokes provide
 * a small red glow without animation or a large shadow redraw area.
 * Coordinates follow the existing 512px face at zoom 425, centered at 360. */
static const lv_point_t rpm_accent_points[] = {{0, 0}, {74, 0}};

static void create_rpm_accents(lv_obj_t *parent)
{
    static const lv_coord_t widths[] = {10, 6, 3};
    static const lv_opa_t opacities[] = {24, 64, LV_OPA_COVER};
    for(unsigned side = 0; side < 2; ++side) {
        for(unsigned layer = 0; layer < 3; ++layer) {
            lv_obj_t *stroke = lv_line_create(parent);
            lv_obj_remove_style_all(stroke);
            lv_obj_set_pos(stroke, side ? 431 : 211, 220);
            lv_line_set_points(stroke, rpm_accent_points, 2);
            lv_obj_set_style_line_width(stroke, widths[layer], 0);
            lv_obj_set_style_line_color(stroke, lv_color_hex(0xFF0000), 0);
            lv_obj_set_style_line_opa(stroke, opacities[layer], 0);
            lv_obj_set_style_line_rounded(stroke, true, 0);
            lv_obj_clear_flag(stroke, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_black(), 0);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    ui_rpm_bg = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_rpm_bg, &face);
    lv_obj_center(ui_rpm_bg);
    /* Fill the 720px circle with the face inside the decorative bezel.
     * The silver ring is outside the screen at this zoom. */
    lv_img_set_zoom(ui_rpm_bg, 425);
    create_rpm_accents(ui_Screen1);

    lv_obj_t *panel = lv_obj_create(ui_Screen1);
    lv_obj_remove_style_all(panel);
    lv_obj_set_pos(panel, 204, 322);
    lv_obj_set_size(panel, 308, 91);
    lv_obj_set_style_radius(panel, 12, 0);
    lv_obj_set_style_bg_color(panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    rpm_label = lv_label_create(panel);
    lv_obj_set_style_text_font(rpm_label, &tach_digits, 0);
    lv_obj_set_style_text_color(rpm_label, lv_color_white(), 0);
    lv_label_set_text(rpm_label, "0");
    lv_obj_center(rpm_label);

    /* Retain the exported name; the pointer is now a narrow radial line.
     * Artwork at zoom 425: inner arch ~224px, outer arch ~352px. */
    ui_rpm_arc = lv_line_create(ui_Screen1);
    lv_obj_remove_style_all(ui_rpm_arc);
    lv_obj_set_pos(ui_rpm_arc, 0, 0);
    lv_obj_set_style_line_width(ui_rpm_arc, 8, 0);
    lv_obj_set_style_line_color(ui_rpm_arc, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_line_opa(ui_rpm_arc, LV_OPA_COVER, 0);
    lv_obj_set_style_line_rounded(ui_rpm_arc, false, 0);
    lv_obj_clear_flag(ui_rpm_arc, LV_OBJ_FLAG_CLICKABLE);
    /* Existing speed/gear/odo callbacks retain valid, hidden targets. */
    lv_obj_t **legacy[] = {&ui_label_mph_value, &ui_label_mph, &ui_label_gear,
                          &ui_label_gear_value, &ui_label_odometer_value};
    for(unsigned i = 0; i < sizeof(legacy) / sizeof(legacy[0]); ++i) {
        *legacy[i] = lv_label_create(ui_Screen1);
        lv_obj_add_flag(*legacy[i], LV_OBJ_FLAG_HIDDEN);
    }
    /* Gear panel below the RPM readout, above the logo divider. */
    lv_obj_t *gear_panel = lv_obj_create(ui_Screen1);
    lv_obj_remove_style_all(gear_panel);
    lv_obj_set_pos(gear_panel, 242, 464);
    lv_obj_set_size(gear_panel, 233, 101);
    lv_obj_set_style_bg_color(gear_panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(gear_panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(gear_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *outline = lv_line_create(gear_panel);
    lv_obj_remove_style_all(outline);
    lv_line_set_points(outline, gear_outline, 7);
    lv_obj_set_style_line_width(outline, 2, 0);
    lv_obj_set_style_line_color(outline, lv_color_hex(0x009FFF), 0);
    /* Reuse the legacy labels as the visible gear UI. */
    lv_obj_set_parent(ui_label_gear, gear_panel);
    lv_obj_clear_flag(ui_label_gear, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_label_gear, "GEAR");
    lv_obj_set_style_text_color(ui_label_gear, lv_color_white(), 0);
    lv_obj_align(ui_label_gear, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_parent(ui_label_gear_value, gear_panel);
    lv_obj_clear_flag(ui_label_gear_value, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(ui_label_gear_value, lv_color_white(), 0);
    last_gear = -99;
    ui_tach_set_gear(-1);
    last_rpm = -1;
    pointer_position_valid = false;
    ui_tach_set_rpm(0);
}

void ui_tach_set_rpm(int rpm)
{
    if(!rpm_label || !ui_rpm_arc) return;
    if(rpm < 0) rpm = 0;
    /* Preserve the actual numeric RPM above full scale; clamp only the marker. */
    if(rpm == last_rpm) return;
    last_rpm = rpm;
    lv_label_set_text_fmt(rpm_label, "%d", rpm);
    lv_obj_center(rpm_label);
    int capped = rpm > 6500 ? 6500 : rpm;
    /* Artwork ticks are not uniformly spaced. Calibrate to their positions. */
    static const int rpm_ticks[] = {0, 1000, 2000, 3000, 4000, 5000, 6000, 6500};
    static const int angle_ticks[] = {180, 198, 221, 246, 276, 304, 334, 360};
    unsigned segment = 0;
    while(segment < 6 && capped > rpm_ticks[segment + 1]) ++segment;
    float angle = angle_ticks[segment] +
        (float)(capped - rpm_ticks[segment]) *
        (angle_ticks[segment + 1] - angle_ticks[segment]) /
        (rpm_ticks[segment + 1] - rpm_ticks[segment]);
    float radians = angle * (3.14159265358979323846f / 180.0f);
    float x = cosf(radians), y = sinf(radians);
    lv_point_t screen_points[2] = {
        {(lv_coord_t)lroundf(360.0f + 224.0f * x),
         (lv_coord_t)lroundf(353.0f + 224.0f * y)},
        {(lv_coord_t)lroundf(360.0f + 350.0f * x),
         (lv_coord_t)lroundf(353.0f + 350.0f * y)}
    };
    if(pointer_position_valid &&
       screen_points[0].x == last_screen_points[0].x &&
       screen_points[0].y == last_screen_points[0].y &&
       screen_points[1].x == last_screen_points[1].x &&
       screen_points[1].y == last_screen_points[1].y) return;

    lv_coord_t left = LV_MIN(screen_points[0].x, screen_points[1].x);
    lv_coord_t top = LV_MIN(screen_points[0].y, screen_points[1].y);
    lv_coord_t width = LV_ABS(screen_points[1].x - screen_points[0].x) + 1;
    lv_coord_t height = LV_ABS(screen_points[1].y - screen_points[0].y) + 1;
    /* Invalidate the previous geometry BEFORE modifying the referenced points. */
    lv_obj_invalidate(ui_rpm_arc);
    for(unsigned i = 0; i < 2; ++i) {
        last_screen_points[i] = screen_points[i];
        pointer_points[i].x = screen_points[i].x - left;
        pointer_points[i].y = screen_points[i].y - top;
    }
    /* Local coordinates keep the object's dirty rectangle around the needle,
     * rather than stretching from screen origin to the needle's far end. */
    lv_obj_set_pos(ui_rpm_arc, left, top);
    lv_obj_set_size(ui_rpm_arc, width, height);
    lv_line_set_points(ui_rpm_arc, pointer_points, 2);
    pointer_position_valid = true;
}

/* -1 = unavailable, 0 = neutral, 1..8 = reported forward gear.
 * Call only in LVGL context; do not infer neutral from a missing speed signal. */
void ui_tach_set_gear(int gear)
{
    if(!ui_label_gear_value) return;
    if(gear < -1 || gear > 8) gear = -1;
    if(gear == last_gear) return;
    last_gear = gear;
    if(gear >= 1) {
        lv_obj_set_style_text_font(ui_label_gear_value, &tach_digits, 0);
        lv_label_set_text_fmt(ui_label_gear_value, "%d", gear);
    } else {
        lv_obj_set_style_text_font(ui_label_gear_value, &ui_font_Doto_48, 0);
        lv_label_set_text(ui_label_gear_value, gear == 0 ? "N" : "--");
    }
    lv_obj_align(ui_label_gear_value, LV_ALIGN_CENTER, 0, 10);
}

void ui_Screen1_screen_destroy(void)
{
    if(ui_Screen1) lv_obj_del(ui_Screen1);
    ui_Screen1 = ui_rpm_bg = ui_rpm_arc = rpm_label = NULL;
    ui_label_mph_value = ui_label_mph = ui_label_gear = NULL;
    ui_label_gear_value = ui_label_odometer_value = NULL;
    last_rpm = -1;
    pointer_position_valid = false;
}
