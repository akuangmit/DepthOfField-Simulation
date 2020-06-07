#ifndef RENDERER_H
#define RENDERER_H

#include <string>

#include "SceneParser.h"
#include "ArgParser.h"

class Hit;
class Vector3f;
class Ray;

class Renderer
{
  public:
    // Instantiates a renderer for the given scene.
    Renderer(const ArgParser &args);
    void Render();
  private:
    Vector3f traceRay(const Ray &ray, float tmin, int bounces, 
                      Hit &hit) const;

    Vector3f traceRayDOF(const Ray &ray, float distToPixel, float distToImg, float focal_length,
                      std::vector<Vector3f> sampledPoints, float tmin, int bounces,
                      Hit &hit, bool jittering = true) const;

    std::vector<Vector3f> getSampledPoints(Camera* cam, float x, float y, float aperture) const;

    ArgParser _args;
    SceneParser _scene;
};

#endif // RENDERER_H
