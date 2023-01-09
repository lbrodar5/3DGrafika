#include <iostream>
#include <cmath>
#include "tgaimage.h"
#include "tgaimage.cpp"
using namespace std;

// dimenzije slike
const int width  = 512;
const int height = 512;

// definirajmo boje
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0, 0, 255);
const TGAColor blue  = TGAColor(0, 0, 255, 255);
const TGAColor green = TGAColor(0,255,0,255);

void set_color(int x, int y, TGAImage &image, TGAColor color, bool invert = false)
{
    image.set(y, x, color);    
}

float line(float x0, float y0, float x1, float y1, float x, float y)
{
    return (y0 - y1) * x + (x1 - x0) * y + x0 * y1 - x1 * y0;
}

void line_naive(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    for (float t = 0; t < 1; t += 0.01)
    {
        int x = x0 * (1.0f - t) + x1 * t;
        int y = x0 * (1.0f - t) + y1 * t;
        set_color(x, y, image, color);
    }
}

void line_midpoint(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color, bool invert)
{
    int y = y0;
    int d = line(x0, y0, x1, y1, x0 + 1, y0 + 0.5);
    
    int dx = (x1 - x0);
    int dy = (y1 - y0);
    int increment = 1;
    
    if (dy < 0)
    {
        // pravac ide od gore prema dolje
        dy = -dy;
        increment = -increment;
    }
    
    for (int x = x0; x <= x1; ++x)
    {
        if (invert)
        {
            set_color(x, y, image, color);
        }
        else
        {
            set_color(y, x, image, color);       
        }
        
        if (d < 0)
        {
            y = y + increment;
            d = d + dx - dy;
        }
        else
        {
            d = d - dy;
        }
    }
}

void draw_line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    // 'transponiraj' duzinu ako je veci od 1
    bool invert = false;
    if (abs(x1 - x0) < abs(y1 - y0))
    {
        swap(x0, y0);
        swap(x1, y1);
        invert = true;
    }
    
    // zamijeni tocke ako pravac ide s desna na lijevo
    if (x1 < x0)
    {
        swap(x0, x1);
        swap(y0, y1);
    }
    
    // nacrtaj duzinu
    line_midpoint(x0, y0, x1, y1, image, color, invert);
}



void draw_tirange_3d(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, TGAImage &image, TGAColor color){

    int maxX = max(x0, max(x1, x2));
    int minX = min(x0, min(x1, x2));
    int maxY = max(y0, max(y1, y2));
    int minY = min(y0, min(y1, y2));
    int maxZ = max(z0, max(z1, z2));
    int minZ = min(z0, min(z1, z2));

    int v1[3] = {x1-x0,y1-y0,z1-z0};
    int v2[3] = {x2-x0,y2-y0,z2-z0};
    int v3[3] = {x2-x1,y2-y1,z2-z1};

    for (int x = minX; x <= maxX; x++){
        for (int y = minY; y <= maxY; y++){
            for(int z = minZ; z <= maxZ; z++){
                float s = (float)((v2[1]-v3[1])*(x-v3[0]) + (v3[0]-v2[0])*(y-v3[1]))/(v2[1]-v3[1])*(v1[0]-v3[0]) + (v3[0]-v2[0])*(v1[1]-v3[1]);
                float t = (float)((v3[1]-v1[1])*(x-v3[0]) + (v1[0]-v3[0])*(y-v3[1]))/(v2[1]-v3[1])*(v1[0]-v3[0]) + (v3[0]-v2[0])*(v1[1]-v3[1]);
                
                
                cout << s << " " << t << "\n";
                if ( (s >= 0) && (t >= 0) && (s + t <= 1))
                {
                    set_color(x,y,image,color);
                }
            }

        }
    } 

}



int main()
{
    // definiraj sliku
    TGAImage image(width, height, TGAImage::RGB);
    
    // nacrtaj nekoliko duzina    

    draw_tirange_3d(10,300,50,300,250,50,50,50,50,image,red);

    // spremi sliku 
    image.flip_vertically();
    image.write_tga_file("lines.tga");
}