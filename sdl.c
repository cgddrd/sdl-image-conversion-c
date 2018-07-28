#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

SDL_Surface *image;
int blur_extent = 2;

// For the flipImage function
enum flipDirection
{
    FLIP_X = 0,
    FLIP_Y = 1
};

// get_pixel: Acquires a 32-bit pixel from a surface at given coordinates
Uint32 get_pixel(SDL_Surface *surface, int x, int y)
{
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;
    //Get the requested pixelSDL_SRCCOLORKEY
    return pixels[(y * surface->w) + x];
}

// put_pixel: Drops a 32-bit pixel onto a surface at given coordinates.
void put_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;
    //Set the pixel
    pixels[(y * surface->w) + x] = pixel;
}

SDL_Surface *create_blank_surface(int width, int height)
{
    SDL_Surface *surface;
    Uint32 rmask, gmask, bmask, amask;

    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    surface = SDL_CreateRGBSurface(0, width, height, 32,
                                   rmask, gmask, bmask, amask);
    if (surface == NULL)
    {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        exit(1);
    }

    /* or using the default masks for the depth: */
    surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
}

SDL_Surface *flipImage(SDL_Surface *origin, enum flipDirection dir)
{
    // Generate a blank surface of the same size to store the
    // flipped image.
    SDL_Surface *ret = create_blank_surface(origin->w, origin->h);

    // x and y are the original pixels, while rx and ry are the reverse,
    // which will be switched by this function.
    for (int x = 0, rx = ret->w - 1; x < ret->w; x++, rx--)
    {
        for (int y = 0, ry = ret->h - 1; y < ret->h; y++, ry--)
        {
            Uint32 pixel = get_pixel(origin, x, y);
            // FLIP_X and FLIP_Y determine which pixels to swap
            if (dir == FLIP_X)
                put_pixel(ret, rx, y, pixel);
            else
                put_pixel(ret, y, ry, pixel);
        }
    }

    return ret;
}

void blur() //This manipulates with SDL_Surface and gives it box blur effect
{
    for (int y = 0; y < image->h; y++)
    {
        for (int x = 0; x < (image->pitch / 4); x++)
        {
            Uint32 color = ((Uint32 *)image->pixels)[(y * (image->pitch / 4)) + x];

            //SDL_GetRGBA() is a method for getting color
            //components from a 32 bit color
            Uint8 r = 0, g = 0, b = 0, a = 0;
            SDL_GetRGBA(color, image->format, &r, &g, &b, &a);

            Uint32 rb = 0, gb = 0, bb = 0, ab = 0;

            //Within the two for-loops below, colors of adjacent pixels are added up

            for (int yo = -blur_extent; yo <= blur_extent; yo++)
            {
                for (int xo = -blur_extent; xo <= blur_extent; xo++)
                {
                    if (y + yo >= 0 && x + xo >= 0 && y + yo < image->h && x + xo < (image->pitch / 4))
                    {
                        Uint32 colOth = ((Uint32 *)image->pixels)[((y + yo) * (image->pitch / 4)) + (x + xo)];

                        Uint8 ro = 0, go = 0, bo = 0, ao = 0;
                        SDL_GetRGBA(colOth, image->format, &ro, &go, &bo, &ao);

                        rb += ro;
                        gb += go;
                        bb += bo;
                        ab += ao;
                    }
                }
            }

            //The sum is then, divided by the total number of
            //pixels present in a block of blur radius

            //For blur_extent 1, it will be 9
            //For blur_extent 2, it will be 25
            //and so on...

            //In this way, we are getting the average of
            //all the pixels in a block of blur radius

            //(((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)) calculates
            //the total number of pixels present in a block of blur radius

            r = (Uint8)(rb / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));
            g = (Uint8)(gb / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));
            b = (Uint8)(bb / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));
            a = (Uint8)(ab / (((blur_extent * 2) + 1) * ((blur_extent * 2) + 1)));

            //Bit shifting color bits to form a 32 bit proper colour
            color = (r) | (g << 8) | (b << 16) | (a << 24);
            ((Uint32 *)image->pixels)[(y * (image->pitch / 4)) + x] = color;
        }
    }
}

void grayscale()
{

    Uint32 *pixels = (Uint32 *)image->pixels;

    for (int y = 0; y < image->h; y++)
    {
        for (int x = 0; x < image->w; x++)
        {
            Uint32 pixel = pixels[y * image->w + x];

            //SDL_GetRGBA() is a method for getting color
            //components from a 32 bit color
            Uint8 r = 0, g = 0, b = 0, a = 0;
            SDL_GetRGBA(pixel, image->format, &r, &g, &b, &a);

            // Uint8 r = pixel >> 16 & 0xFF;
            // Uint8 g = pixel >> 8 & 0xFF;
            // Uint8 b = pixel & 0xFF;

            Uint8 v = 0.212671f * r + 0.715160f * g + 0.072169f * b;

            pixel = (0xFF << 24) | (v << 16) | (v << 8) | v;
            pixels[y * image->w + x] = pixel;
        }
    }
}

int main(int argc, char **argv)
{
    int success = 0;

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG);
    SDL_Window *window = SDL_CreateWindow("SDL2 Grayscale", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    image = SDL_LoadBMP("test.bmp");

    if (image == NULL)
    {
        printf("Unable to load bitmap: %s\n", SDL_GetError());
        return 2;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
                                             image->w, image->h);

    SDL_UpdateTexture(texture, NULL, image->pixels,
                      image->w * sizeof(Uint32));

    image = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ARGB8888, 0);

    Uint32 *pixels = (Uint32 *)image->pixels;
    blur();
    grayscale();

    SDL_Surface *image2 = flipImage(image, FLIP_X);

    //SDL_Surface * image = IMG_Load("img.png");
    //IMG_SavePNG(image, "out.png");
    SDL_SaveBMP(image2, "out.bmp");
    SDL_FreeSurface(image2);

    return 0;
}