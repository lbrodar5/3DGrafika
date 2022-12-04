#include <cmath>
#include <vector>
#include <fstream>
#include <algorithm>
#include "geometry.h"
#include "ray.h"
#include "objects.h"
#include "light.h"

using namespace std;

typedef vector<Vec3f> Image;
typedef vector<Object*> Objects;
typedef vector<Light*> Lights;

// funkcija koja ispisuje sliku u .ppm file
void save_image(const Image &image, const int width, const int height, const string path)
{
    ofstream ofs;
    ofs.open(path, ofstream::binary);
    
    // format ppm
    ofs << "P6\n" << width << " " << height << "\n255\n";
    
    // ispis pixela
    for (int i = 0; i < width * height; ++i)
    {
        ofs << (char)(255 * min(max(image[i][0], 0.f), 1.f));
        ofs << (char)(255 * min(max(image[i][1], 0.f), 1.f));
        ofs << (char)(255 * min(max(image[i][2], 0.f), 1.f));
    }
    
    // zatvori file
    ofs.close(); 
}

// funkcija koja provjerava sijece li zraka jedan od objekata
bool scene_intersect(const Ray &ray, const Objects &objs, Material &hit_material, Vec3f &hit_point, Vec3f &hit_normal)
{
    float best_dist = numeric_limits<float>::max();
    float dist = numeric_limits<float>::max();
    
    Vec3f normal;
    
    for (auto obj : objs)
    {
        if (obj->ray_intersect(ray, dist, normal) && dist < best_dist)
        {
            best_dist = dist;             // udaljenost do sfere
            hit_material = obj->material; // materijal pogodjene sfere
            hit_normal = normal;          // normala na pogodjeni objekt
            hit_point = ray.origin + ray.direction * dist; // pogodjena tocka
        }
    }
    
    return best_dist < 1000;
}

// funkcija koja vraca boju
Vec3f cast_ray(const Ray &ray, const Objects &objs, const Lights &lights, int depth)
{
    Vec3f hit_normal;
    Vec3f hit_point;
    Material hit_material;

    Vec3f boja_pozadine = Vec3f(0.8, 0.8, 1);
    
    if(depth == 0){
        return Vec3f(0,0,0);
    }
    if (!scene_intersect(ray, objs, hit_material, hit_point, hit_normal))
    {
        return  boja_pozadine; // vrati boju pozadine
    }
    {
        float diffuse_light_intensity = 0;
        Vec3f ref_color(0,0,0); 
        
        for (auto light : lights) 
        {           
            Vec3f light_dir = (light->position - hit_point).normalize();
            float light_dist = (light->position - hit_point).norm();
            
            // SJENE
            // ideja: - rekurzivno pozovi scene_intersect od objekta do svijetla
            //        - ako se nesto nalazi izmedju svjetla i objekta, 
            //          tada to svijetlo ignoriramo
            Material shadow_hit_material;
            Vec3f shadow_hit_normal;
            Vec3f shadow_hit_point;
            
            // zbog gresaka u zaokrizivanju moze se dogoditi da zraka zapocne
            // unutar samog objekta. Da to izbjegnemu, origin zrake  za malo
            // pomicemo u smjeru zrake
            Vec3f shadow_origin;
            if (light_dir * hit_normal < 0) // skalarni produkt je manji od 0 ako su suprotne orijentacije
            {                               
                shadow_origin = hit_point - hit_normal * 0.001;
            }
            else
            {
                shadow_origin = hit_point + hit_normal * 0.001;
            }
            Ray shadow_ray(shadow_origin, light_dir);
            
            // provjeri hoce li zraka shadow_ray presijecatiobjekt
            if (scene_intersect(shadow_ray, objs, shadow_hit_material, shadow_hit_point, shadow_hit_normal))
            {
                // zraka sijece neki objekt 
                // trebamo jos provjeriti zaklanja li taj objekt svjetlo
                // tj. nalazi li se izmedju hit_point i light->position
                float dist = (shadow_hit_point - hit_point).norm();
                if (dist < light_dist)
                {
                    // objekt zaklanja svijetlo, preskacemo ovu iteraciju
                    continue;
                }
            }
            
            // I / r^2
            float dist_factor = light->intensity / (light_dist * light_dist);

            // difuzno sjenacanje (Lambertov model)
            diffuse_light_intensity +=  hit_material.diffuse_coef * dist_factor * max(0.f, hit_normal * light_dir);
        }

        
        Vec3f diffuse_color = hit_material.diffuse_color * diffuse_light_intensity;

        //opacitiy
         if(hit_material.opacity < 1){
            Ray opacityRay = Ray(hit_point+(ray.direction*0.001),ray.direction);
            Vec3f o = cast_ray(opacityRay,objs,lights,depth);
             diffuse_color = diffuse_color + o*(1-hit_material.opacity);
            
         }


        //refelksija
        Vec3f r1 = hit_normal*((hit_point*hit_normal)*2);

        Vec3f r = (hit_point - r1).normalize();

        Ray refRay = Ray(hit_point+(hit_normal*0.001),r);

        Vec3f ref = cast_ray(refRay,objs,lights,depth-1);

        ref_color = ref + ref_color;
        
        diffuse_color = diffuse_color + ref_color*hit_material.ref_coef;


        return diffuse_color;
    }
}

// funkcija koja napravi zraku iz točke origin
// koja prolazi kroz pixel (i, j) na slici
// (formula s predavanja 3)
Ray ray_to_pixel(Vec3f origin, int i, int j, int width, int height)
{
    Ray ray = Ray();
    ray.origin = origin;
    
    float fov = 1.855; // 106.26° u radijanima
    float tg = tan(fov / 2.);
    
    float x =  (-1 + 2 * (i + 0.5) / (float)width) * tg;
    float y = -(-1 + 2 * (j + 0.5) / (float)height);
    float z = -1;
    
    ray.direction = Vec3f(x, y, z).normalize();
    return ray;
}

void draw_image(Objects objs, Lights lights)
{
    // dimenzije slike
    const int width = 1024;
    const int height = 768;
    
    Image img(width * height);
    
    // ishodište zrake
    Vec3f origin = Vec3f(0, 0, 0);
    
    // crtanje slike, pixel po pixel
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            Ray ray = ray_to_pixel(origin, i, j, width, height);
            Vec3f color = cast_ray(ray, objs, lights, 2);
            img[i + j * width] = color;
        }
    }
    
    // snimi sliku na disk
    save_image(img, width, height, "./render.ppm");
}

int main()
{
    // definiraj materijale
    Material red(Vec3f(1, 0, 0));
    red.ref_coef = 0.5;
    
    Material green(Vec3f(0, 1, 0));
    green.ref_coef = 0;
    green.opacity = 0.1;

    Material blue(Vec3f(0, 0, 1));

    Material grey(Vec3f(0.2, 0.2, 0.2));
    grey.ref_coef = 0.3;

    Material purple(Vec3f(1,0,1));
    purple.opacity = 0.5;
    purple.ref_coef = 0;

    
    // definiraj objekte u sceni
    Sphere s1(Vec3f(-2.5,    0,   -16), 2, red);
    Sphere s2(Vec3f(5.0, -1.5, -12), 2, green);
    Sphere s3(Vec3f( 3, -0.5, -18), 3, blue);
    Sphere s4(Vec3f( 0,    5,   -20), 4, grey);
    Cuboid c1(Vec3f(-15,-5,-30), Vec3f(15,-4,-7), grey);
    Cuboid c2(Vec3f(-8,-3,-16),Vec3f(-6,1,-13), blue);
    Cuboid c3(Vec3f(-4,-5,-12),Vec3f(-1,-1,-10),purple);
    Objects objs = {&s1, &s2, &s3, &s4, &c1, &c2, &c3};
    
    // definiraj svjetla 
    Light l1 = Light(Vec3f(10, 10, 10), 1000);
    Light l2 = Light(Vec3f(-10, 10, 10), 700);

    Lights lights = {&l1, &l2};
    
    draw_image(objs, lights);
    return 0;
}