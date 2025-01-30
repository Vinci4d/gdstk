/*
Copyright 2020 Lucas Heitzmann Gabrielli.
This file is part of gdstk, distributed under the terms of the
Boost Software License - Version 1.0.  See the accompanying
LICENSE file or <http://www.boost.org/LICENSE_1_0.txt>
*/

#ifndef GDSTK_HEADER_POLYGON
#define GDSTK_HEADER_POLYGON

#define __STDC_FORMAT_MACROS 1
#define _USE_MATH_DEFINES

#include <stdint.h>
#include <stdio.h>

#include <vector>
#include <memory>
#include <string>

#include "array.hpp"
#include "oasis.hpp"
#include "property.hpp"
#include "repetition.hpp"
#include "utils.hpp"
#include "vec.hpp"

namespace gdstk {

struct Polygon {
    Tag tag;
    Array<Vec2> point_array;
    Repetition repetition;
    Property* properties;
    // Used by the python interface to store the associated PyObject* (if any).
    // No functions in gdstk namespace should touch this value!
    void* owner;

    void print(bool all) const;

    void clear();

    // This polygon instance must be zeroed before copy_from
    void copy_from(const Polygon& polygon);

    // Total polygon area including any repetitions
    double area() const;

    // Polygon area excluding repetitions with sign indicating orientation
    // (positive for counter clockwise)
    double signed_area() const;
    
    // Total polygon perimeter including any repetitions
    double perimeter() const;

    // Check if the points are inside this polygon (points lying on the edges
    // or coinciding with a vertex of the polygon are considered inside).
    bool contain(const Vec2 point) const;
    bool contain_all(const Array<Vec2>& points) const;
    bool contain_any(const Array<Vec2>& points) const;

    // Bounding box corners are returned in min and max.  If the polygons has
    // no vertices, return min.x > max.x.  Repetitions are taken into account
    // for the calculation.
    void bounding_box(Vec2& min, Vec2& max) const;

    void translate(const Vec2 v);
    void scale(const Vec2 scale, const Vec2 center);
    void mirror(const Vec2 p0, const Vec2 p1);
    void rotate(double angle, const Vec2 center);

    // Transformations are applied in the order of arguments, starting with
    // magnification and translating by origin at the end.  This is equivalent
    // to the transformation defined by a Reference with the same arguments.
    void transform(double magnification, bool x_reflection, double rotation, const Vec2 origin);

    // Round the corners of this polygon.  Argument radii can include one
    // radius value for each polygon corner or less, in which case it will be
    // cycled.  If the desired fillet radius for a given corner is larger than
    // half the shortest edge adjacent to that corner, it is reduced to that
    // size.  The number of vertices used to approximate the circular arcs is
    // defined by tolerance.
    void fillet(const Array<double> radii, double tolerance);

    // Fracture the polygon horizontally and vertically until all pieces have
    // at most max_points vertices.  If max_points < 5, it doesn't do anything.
    // Resulting pieces are appended to result.
    void fracture(uint64_t max_points, double precision, Array<Polygon*>& result) const;

    // Append the copies of this polygon defined by its repetition to result.
    void apply_repetition(Array<Polygon*>& result);
    void apply_repetition_no_clear(Array<Polygon*>& result) const;

    // These functions output the polygon in the GDSII, OASIS and SVG formats.
    // They are not supposed to be called by the user.
    ErrorCode to_gds(FILE* out, double scaling) const;
    ErrorCode to_oas(OasisStream& out, OasisState& state) const;
    ErrorCode to_svg(FILE* out, double scaling, uint32_t precision) const;
};

Polygon rectangle(const Vec2 corner1, const Vec2 corner2, Tag tag);

Polygon cross(const Vec2 center, double full_size, double arm_width, Tag tag);

// The polygon is created with a horizontal lower edge when rotation is 0.
Polygon regular_polygon(const Vec2 center, double side_length, uint64_t sides, double rotation,
                        Tag tag);

// Create circles, ellipses, rings, or sections of those.  The number of points
// used to approximate the arcs is such that the approximation error is less
// than tolerance.
Polygon ellipse(const Vec2 center, double radius_x, double radius_y, double inner_radius_x,
                double inner_radius_y, double initial_angle, double final_angle, double tolerance,
                Tag tag);

Polygon racetrack(const Vec2 center, double straight_length, double radius, double inner_radius,
                  bool vertical, double tolerance, Tag tag);

// Create a polygonal text form NULL-terminated string s.  Argument size
// defines the full height of the glyphs.  Polygons are appended to result.
// The character aspect ratio is 1:2.  For horizontal text, spacings between
// characters and between lines are 9/16 and 5/4 times the full height size,
// respectively.  For vertical text, characters and columns are respectively
// spaced by 9/8 and 1 times size.
void text(const char* s, double size, const Vec2 position, bool vertical, Tag tag,
          Array<Polygon*>& result);

// Create polyogns based on the 2-d array data (in row-major orderr, with rows
// * cols elements) by drawing the isolines at level.  Scaling is used in the
// boolean composition of resulting shapes to connect any holes and set the
// overall precision.  Resulting polygons are appended to result.  Their length
// scale is one data element, i.e., the data array has size cols × rows.
ErrorCode contour(const double* data, uint64_t rows, uint64_t cols, double level, double scaling,
                  Array<Polygon*>& result);

// Check if the points are inside a set of polygons (points lying on the edges
// or coinciding with a vertex of the polygons are considered inside).  Result
// must be an array with size for at least points.count bools.
void inside(const Array<Vec2>& points, const Array<Polygon*>& polygons, bool* result);
bool all_inside(const Array<Vec2>& points, const Array<Polygon*>& polygons);
bool any_inside(const Array<Vec2>& points, const Array<Polygon*>& polygons);

struct RepetitionInfo {

    RepetitionType type;

    struct {               // Rectangular and Regular
        uint64_t columns;  // Along x or v1
        uint64_t rows;     // Along y or v2
        union {
            double spacing[2];  // Rectangular spacing
            struct {
                double v1[2];  // Regular axis 1
                double v2[2];  // Regular axis 2
            };
        };
    };
    std::vector<double> offsets;   // Explicit
    std::vector<double> coords;    // ExplicitX and ExplicitY

    void operator=(const Repetition &r) {
        type    = r.type;
        switch(type) {
            case RepetitionType::Rectangular: {
                columns = r.columns;
                rows    = r.rows;
                spacing[0] = r.spacing.x;
                spacing[1] = r.spacing.y;
            } break;
            case RepetitionType::Regular: {
                columns = r.columns;
                rows    = r.rows;
                v1[0] = r.v1.x;      v1[1] = r.v1.y;
                v2[0] = r.v2.x;      v2[1] = r.v2.y;
            } break;
            case RepetitionType::Explicit: {
                offsets.resize(r.offsets.count*2);
                for(size_t i=0; i<r.offsets.count; i++) {
                    offsets[i*2  ] = r.offsets[i].x;
                    offsets[i*2+1] = r.offsets[i].y;
                }
            } break;
            case RepetitionType::ExplicitX:
            case RepetitionType::ExplicitY: {
                coords.resize(r.coords.count);
                for(size_t i=0; i<r.coords.count; i++) {
                    coords[i] = r.coords[i];
                }
            } break;
            default: break;
        }
    }
};

// intermediate tree, built from Cell::build_polygon_tree, Reference::build_polygon_tree
struct OffsetPolyTree
{
    int depth = 0;
    
    struct OffsettedPolys {
        uint64_t tag;       // datatype, layer_number
        std::vector<double> points;
        std::vector<double> offsets;

        void copy(Polygon *polygon) {
            tag = polygon->tag;
            points.resize(polygon->point_array.count*2);
            for(uint64_t i=0; i<polygon->point_array.count; i++) {
                points[i*2  ] = polygon->point_array[i].x;
                points[i*2+1] = polygon->point_array[i].y;
            }
            offsets.clear();
            if (polygon->repetition.type != RepetitionType::None) {
                Array<Vec2> g_offsets = {};
                polygon->repetition.get_offsets(g_offsets);
                offsets.resize(g_offsets.count*2);
                for (uint64_t i=0; i<g_offsets.count; i++) {
                    offsets[i*2  ] = g_offsets.items[i].x;
                    offsets[i*2+1] = g_offsets.items[i].y;
                }
                g_offsets.clear();
            }
        };
    };

    std::vector<OffsettedPolys> offsetted_polys;
    std::vector<double> offsets;
    RepetitionInfo repetitionInfo;

    bool   ref_node = false;
    double ref_node_origin[2] = {0,0};
    double ref_node_magnification = 1;
    bool   ref_node_x_reflection  = false;
    double ref_node_rotation      = 0;

    //
    std::vector<std::shared_ptr<OffsetPolyTree>> ch;

    void print(const std::string &ind, int chid = -1) const {
        if(chid == -1) {
            printf("%s[root, ", ind.c_str());
        } else {
            printf("%s[ch%d, ", ind.c_str(), chid);
        }
        if(ref_node) {
            printf("reference]\n");
        } else {
            printf("cell]\n");
        }

        printf("%s  #offsetted_polys = %d\n", ind.c_str(), (int)offsetted_polys.size());
        printf("%s  #offsets         = %d\n", ind.c_str(), (int)offsets.size()/2);
        printf("%s  origin = %lf,%lf\n", ind.c_str(), ref_node_origin[0], ref_node_origin[1]);
        printf("%s  magnification = %lf\n", ind.c_str(), ref_node_magnification);
        printf("%s  rotation = %lf\n", ind.c_str(), ref_node_rotation);
        printf("%s  x_reflection = %s\n", ind.c_str(), ref_node_x_reflection ? "true" : "false");

        for(size_t i=0; i<ch.size(); i++) {
            ch[i]->print(ind + "    ", i);
        }
    }
};

}  // namespace gdstk

#endif
