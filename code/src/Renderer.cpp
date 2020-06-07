#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"
#include "stdio.h"

#include <limits>


Renderer::Renderer(const ArgParser &args) :
    _args(args),
    _scene(args.input_file)
{
}

void
Renderer::Render()
{
    int w = _args.width;
    int h = _args.height;

    int aperture = _args.aperture;
    int focal_length = _args.focal_length;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);

    // loop through all the pixels in the image
    // generate all the samples

    // This look generates camera rays and calls traceRay.
    // It also write to the color, normal, and depth images.
    // You should understand what this code does.
    Camera* cam = _scene.getCamera();
    for (int y = 0; y < h; ++y) {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x) {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;

            // Use PerspectiveCamera to generate a ray.
            Vector2f imagePoint(ndcx, ndcy);

            Ray r = cam->generateRay(imagePoint);
            float distToPixel = cam->distanceToPixel(imagePoint);
            float distToImg = cam->distanceToImage();

            std::vector<Vector3f> sampledPoints = getSampledPoints(cam, ndcx, ndcy, aperture);

            bool chromatic = _args.chromatic_aberration;
            
            Vector3f color;
            Hit h;
            if (chromatic) {
                // colors are red, green, blue
                float testFocalScale = 20;

                // assume that focal length specified is for green color
                float refracIndexR = 1.50917;
                float refracIndexG = 1.51534;
                float refracIndexB = 1.51690;

                float focalB = refracIndexG / refracIndexB * focal_length;
                float focalR = refracIndexG / refracIndexR * focal_length;
                float focalG = focal_length;

                Hit hG;
                Hit hR;
                Hit hB;
                Vector3f colorG = traceRayDOF(r, distToPixel, distToImg, focalG,
                                    sampledPoints, cam->getTMin(), _args.bounces, hG, true);
                Vector3f colorR = traceRayDOF(r, distToPixel, distToImg, (focalR - focal_length) * testFocalScale + focal_length,
                                    sampledPoints, cam->getTMin(), _args.bounces, hR, true);
                Vector3f colorB = traceRayDOF(r, distToPixel, distToImg, (focalB - focal_length) * testFocalScale + focal_length,
                                    sampledPoints, cam->getTMin(), _args.bounces, hB, true);

                std::cout << (focalR - focal_length) * testFocalScale + focal_length << std::endl;
                std::cout << (focalB - focal_length) * testFocalScale + focal_length << std::endl;

                color = Vector3f(colorR.x(), colorG.y(), colorB.z());
            } else if (_args.depth_of_field) {
                color = traceRayDOF(r, distToPixel, distToImg, focal_length,
                                    sampledPoints, cam->getTMin(), _args.bounces, h, true);
            } else {
                color = traceRay(r, cam->getTMin(), _args.bounces, h);
            }

            image.setPixel(x, y, color);
            nimage.setPixel(x, y, (h.getNormal() + 1.0f) / 2.0f);
            float range = (_args.depth_max - _args.depth_min);
            if (range) {
                dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));
            }
        }
    }
    // END SOLN

    // save the files 
    if (_args.output_file.size()) {
        image.savePNG(_args.output_file);
    }
    if (_args.depth_file.size()) {
        dimage.savePNG(_args.depth_file);
    }
    if (_args.normals_file.size()) {
        nimage.savePNG(_args.normals_file);
    }
}



Vector3f
Renderer::traceRay(const Ray &r,
    float tmin,
    int bounces,
    Hit &h) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.

    if (_scene.getGroup()->intersect(r, tmin, h)) {
        Vector3f finalIllum(0,0,0);
        // ambient light
        finalIllum += (_scene.getAmbientLight() * h.getMaterial()->getDiffuseColor());

        // non ambient light sources
        for (int l = 0; l < _scene.getNumLights(); l++) {
            Light* light = _scene.getLight(l);
            Vector3f intersectionP = r.pointAtParameter(h.getT());

            Vector3f tolight;
            Vector3f intensity; 
            float distToLight;

            light->getIllumination(intersectionP, tolight, intensity, distToLight);

            // only count light source if there is uninterrupted line
            Ray shadowR(intersectionP, tolight);
            Hit shadowH;
            if (!_scene.getGroup()->intersect(shadowR, 0.01, shadowH)) {
                Vector3f phong = h.getMaterial()->shade(r, h, tolight, intensity);
                finalIllum += phong;
            }
            
        }

        // bounces
        if (bounces > 0) {
            Vector3f normH = h.getNormal();
            Vector3f eyeDir = - r.getDirection();
            Vector3f eyeReflection = - eyeDir + 2 * (Vector3f::dot(eyeDir, normH)) * normH;

            Ray newR(r.pointAtParameter(h.getT()) , eyeReflection.normalized());
            Hit nextH;
            Vector3f indLight = traceRay(newR, 0.01, bounces - 1, nextH);

            for (int i = 0; i < 3; i++) {
                finalIllum[i] += indLight[i] * h.getMaterial()->getSpecularColor()[i];
            }
        }
        

        return finalIllum;
    } else {
        return _scene.getBackgroundColor(r.getDirection());
    };
}

Vector3f
Renderer::traceRayDOF(const Ray &r,
    float distToPixel,
    float distToImg,
    float focalLength,
    std::vector<Vector3f> sampledPoints,
    float tmin,
    int bounces,
    Hit &h, bool jittering) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.

    // intersect with plane, find P
    // equation from http://cg.skeelogy.com/depth-of-field-using-raytracing/

    Vector3f focalPoint = r.getOrigin() + (distToPixel * (distToImg + focalLength) / distToImg) * r.getDirection();
    Vector3f brightness(0,0,0);
    int i = 0;
    int midPoint = (sampledPoints.size() - 1) / 2;
    for (int i = 0; i < sampledPoints.size(); i++) {
        Vector3f jitteringDiff(0,0,0);
        if (jittering) {
            // attempts at jittering
            float SCALING = 10;
            float dx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) / SCALING;
            float dy = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) / SCALING;
            float dz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) / SCALING;
            jitteringDiff = Vector3f(dx, dy, dz);
        }
        
        Vector3f point = sampledPoints.at(i) + jitteringDiff;
        Ray pixelToF(point, (focalPoint - point).normalized());

        Hit newHit;
        brightness = brightness + traceRay(pixelToF, tmin, bounces, newHit);

        // maybe TODO - making midpoint h is perhaps not a good idea
        if (i == midPoint) {
            h = newHit;
        }
    }

    return brightness / sampledPoints.size();
}


// this should return everything within bound
// TODO figure out some sort of bound if we want to
std::vector<Vector3f>
Renderer::getSampledPoints(Camera* cam, float x, float y, float aperture) const {
    std::vector<Vector3f> res;

    // TODO right now all differences are set, do different sampling
    int size = (aperture - 1) / 2;
    float scale = 0.1;
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {
            Vector3f point3D = cam->getPointOnImagePlane(Vector2f(x + i * scale, y + j * scale));
            res.push_back(point3D);
        }
    }

    return res;
}

