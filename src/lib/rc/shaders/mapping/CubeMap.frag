
uniform vec3 offset;

void swap(inout float t0, inout float t1) {
    float t = t0;
    t0 = t1;
    t1 = t;
}

float intersection(out vec3 uvw) {
    vec3 orig = uvw_vertex_position - offset;
    vec3 dir = normalize(orig);
  float tmin = (-0.5 - orig.x) / dir.x;
   float tmax = (0.5 - orig.x) / dir.x;

   if (tmin > tmax) {
       swap(tmin, tmax);
   }

   float tymin = (-0.5 - orig.y) / dir.y;
   float tymax = ( 0.5 - orig.y) / dir.y;

   if (tymin > tymax) swap(tymin, tymax);

   if ((tmin > tymax) || (tymin > tmax)) {
       return -1.0;
   }

   if (tymin > tmin) {
       tmin = tymin;
   }

   if (tymax < tmax) {
       tmax = tymax;
   }

   float tzmin = ( 0.5 - orig.z) / dir.z;
   float tzmax = (-0.5 - orig.z) / dir.z;

   if (tzmin > tzmax) {
       swap(tzmin, tzmax);
   }

   if ((tmin > tzmax) || (tzmin > tmax)) {
       return -1.0;
   }

   if (tzmin > tmin) {
       tmin = tzmin;
   }

   if (tzmax < tmax) {
       tmax = tzmax;
   }

   uvw = orig + tmax * dir;

   return 1.0;
}

void transform_to_cubemap(inout vec2 texCoords, float offset) {
  float eps =  -0.000;

  texCoords = vec2(texCoords.s/(12.0 - eps) + (0.5 + offset) / 6.0,0.5 + texCoords.t/(2.0 - eps));
}

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define NO_AXIS 3

int dominant_axis(vec3 uvw) {
    vec3 v = abs(uvw);
    if (v.x >= v.y && (v.x >= v.z)) return X_AXIS;
    if (v.y >= v.x && (v.y >= v.z)) return Y_AXIS;
    if (v.z >= v.x && (v.z >= v.y)) return Z_AXIS;
    return NO_AXIS;
}

float mapping(in vec3 uvw, out vec2 texCoords)
{
  float _off = 0.0;
  vec3 v = abs(uvw);
  int axis = dominant_axis(uvw);

  if (axis == X_AXIS) {
    _off = (uvw.x > 0.0) ? 2.0 : 3.0; // EAST / WEST
    texCoords = uvw.yz / abs(uvw.x);
    if (uvw.x > 0.0) {
        _off = 2.0; // EAST
    } else {
        texCoords.s = -texCoords.s;
        _off = 3.0; // WEST
    }
  }
  if (axis == Y_AXIS)
  {
    texCoords = uvw.xz / abs(uvw.y);
    if (uvw.y > 0.0) {
        _off = 0.0; // NORTH
        texCoords.s = - texCoords.s;
    } else {
        _off = 1.0; // SOUTH
    }
  }

  if (axis == Z_AXIS)
  {
    texCoords = uvw.xy / abs(uvw.z);
    if (uvw.z > 0.0) {
        _off = 4.0; // TOP
    } else {
        _off = 5.0; // BOTTOM
        texCoords.t = - texCoords.t;
    }
  }
  transform_to_cubemap(texCoords,_off);

  return 1.0;
}