//
//  ppmgen.cpp
//  ppmgen
//

#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include <random>

// Convience function, since C++ does not have the python equivalent of a range function.
std::vector<int> range(int low, int high)
{
    std::vector<int> retVal;
    for(int i = low; i < high; i++)
    {
        retVal.push_back(i);
    }
    return retVal;
}

// Convenience class for generating random numbers.
class Random
{
public:
    Random() = default;
    Random(std::mt19937::result_type seed) : eng(seed) {}
    uint32_t DrawNumber(uint32_t min, uint32_t max)
    {
        return std::uniform_int_distribution<uint32_t>{min, max}(eng);
    }
    bool DrawBool()
    {
        return std::uniform_int_distribution<uint32_t>{0, 2}(eng) == 1 ? true : false;
    }
private:
    std::mt19937 eng{std::random_device{}()};
};

struct Color
{
    int r, g, b;
};

Color randColor(Random &r)
{
    Color c{};
    c.r = r.DrawNumber(0, 256);
    c.g = r.DrawNumber(0, 256);
    c.b = r.DrawNumber(0, 256);
    return c;
}

void point(std::vector<int> &grid, int x, int y, const Color &c)
{
    if(x > 0 && x < 256 && y > 0 && y < 256)
    {
        int loc = 3 * ((y << 8) + x);
        grid[loc  ] = c.r;
        grid[loc+1] = c.g;
        grid[loc+2] = c.b;
    }
}

std::tuple<int, int, int> getPoint(std::vector<int> &grid, int x, int y)
{
    int loc = 3 * ((y << 8) + x);
    return std::make_tuple(grid[loc], grid[loc+1], grid[loc+2]);
}

void box(std::vector<int> &grid, int x, int y, int w, int h, const Color &c)
{
    for(int dx : range(x, x+w))
    {
        point(grid, dx, y, c);
        point(grid, dx, y+h-1, c);
    }
    for(int dy : range(y+1, y+h-1))
    {
        point(grid, x, dy, c);
        point(grid, x+w-1, dy, c);
    }
}

void solid_box(std::vector<int> &grid, int x, int y, int w, int h, const Color &c)
{
    for(int dx : range(x, x+w))
    {
        for(int dy : range(y, y+h))
        {
            point(grid, dx, dy, c);
        }
    }
}

template<class T>
void swapConditional(T &a, T &b)
{
    if(a > b)
    {
        std::swap(a, b);
        // swap could be implemented like below:
        //
        // T &tmp = a;
        // a = b;
        // b = tmp;
    }
    // else do nothing...
}

void line(std::vector<int> &grid, int x1, int y1, int x2, int y2, const Color &c)
{
    swapConditional(x1, x2);
    swapConditional(y1, y2);
    int w = x2-x1;
    int h = y2-y1;
    
    if(w == 0 && w == h) return;
    
    if(w < h)
    {
        float m = (float)w / (float)h;
        for(int dy : range(0, h))
        {
            int dx = int(float(dy) * m);
            point(grid, x1+dx, y1+dy, c);
        }
    }
    else
    {
        float m = (float)h / (float)w;
        for(int dx : range(0, w))
        {
            int dy = int(float(dx) * m);
            point(grid, x1+dx, y1+dy, c);
        }
    }
}

void circle(std::vector<int> &grid, int x0, int y0, int radius, const Color &c, bool solid = false)
{
    int x = radius;
    int y = 0;
    int radiusError = 1 - x;
    
    while(x >= y)
    {
        if(solid == false)
        {
            point(grid, x+x0,  y+y0, c);
            point(grid, y+x0,  x+y0, c);
            point(grid,-x+x0,  y+y0, c);
            point(grid,-y+x0,  x+y0, c);
            point(grid,-x+x0, -y+y0, c);
            point(grid,-y+x0, -x+y0, c);
            point(grid, x+x0, -y+y0, c);
            point(grid, y+x0, -x+y0, c);
        }
        else
        {
            line(grid, -y+x0, -x+y0, y+x0, -x+y0, c);
            line(grid, -x+x0, -y+y0, x+x0, -y+y0, c);
            line(grid, -x+x0,  y+y0, x+x0,  y+y0, c);
            line(grid, -y+x0,  x+y0, y+x0,  x+y0, c);
        }
        
        y++;
        if(radiusError < 0)
        {
            radiusError += (2 * y + 1);
        }
        else
        {
            x = x - 1;
            radiusError += (2 * (y - x + 1));
        }
    }
}

std::string buildGrid(std::vector<int> &grid, int x, int y, Color &c)
{
    Random r;
    
    std::stringstream ppmData;
    // create the header
    ppmData << "P3\n" << x << " " << y << "\n255\n";
    for(int i = 0; i < 5; ++i)
    {
        c = randColor(r);
        line(grid, r.DrawNumber(0, 256), r.DrawNumber(0, 256), r.DrawNumber(0, 256), r.DrawNumber(0, 256), c);
        
        int rx = r.DrawNumber(0, 255);
        int ry = r.DrawNumber(0, 255);
        box(grid, rx, ry, r.DrawNumber(1, 256 - rx), r.DrawNumber(1, 256 - ry), c);
        
        rx = r.DrawNumber(0, 255);
        ry = r.DrawNumber(0, 255);
        solid_box(grid, rx, ry, r.DrawNumber(1, 256 - rx), r.DrawNumber(1, 256 - ry), c);
        
        rx = r.DrawNumber(55, 255);
        ry = r.DrawNumber(55, 255);
        int radius = r.DrawNumber(3, 255);
        circle(grid, rx, ry, radius, c, r.DrawBool());
    }
    
    // convert list to text file.
    for(int dy : range(0, y))
    {
        for(int dx : range(0, x))
        {
            int a, b, c;
            std::tie(a, b, c) = getPoint(grid, dx, dy);
            ppmData << " " << a << " " << b << " " << c << '\n';
        }
    }
    std::string s(ppmData.str());
    return s;
}

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "PPM Generator\n";
    
    std::vector<int> grid(256*256*3, 0);
    Color c{255, 255, 255};
    std::string data = buildGrid(grid, 256, 256, c);
    
    {
        std::fstream f("./image.ppm", std::ios::out);
        f << data;
    }
    
    
    return 0;
}
