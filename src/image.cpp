#include <SDL2/SDL.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_image.h>
#include "nucleas.h"
#include "rsrc.h"
#include "image.h"
#include "win3d.h"
#include "utils.h"

#include "glob_funcs.h"

const NewClassDescr NC_STACK_image::description("image.class", &newinstance);

NC_STACK_image::NC_STACK_image()
{
}

NC_STACK_image::~NC_STACK_image()
{
}

NC_STACK_image *NC_STACK_image::CInit(IDVList *stak)
{
    NC_STACK_image *tmp = new NC_STACK_image();
    if (!tmp)
        return NULL;

    if (!tmp->func0(stak))
    {
        delete tmp;
        return NULL;
    }

    return tmp;
}

size_t NC_STACK_image::func0(IDVList *stak)
{
    if ( !NC_STACK_bitmap::func0(stak) )
        return 0;

    return 1;
}



static Sint64 mysizefunc(SDL_RWops * context)
{
    FSMgr::FileHandle *fil = (FSMgr::FileHandle *)context->hidden.unknown.data2;

    size_t curr = fil->tell();
    fil->seek(0, SEEK_END);
    size_t sz = fil->tell();
    fil->seek(curr, SEEK_SET);
    return sz;
}

static Sint64 myseekfunc(SDL_RWops *context, Sint64 offset, int whence)
{
    FSMgr::FileHandle *fil = (FSMgr::FileHandle *)context->hidden.unknown.data2;
    switch (whence)
    {
    case RW_SEEK_SET:
        if ( fil->seek(offset, SEEK_SET) != 0 )
            return -1;
        break;
    case RW_SEEK_CUR:
        if ( fil->seek(offset, SEEK_CUR) != 0 )
            return -1;
        break;
    case RW_SEEK_END:
        if ( fil->seek(offset, SEEK_END) != 0 )
            return -1;
        break;
    default:
        SDL_SetError("Can't seek of this kind of whence");
        return -1;
    }
    return fil->tell();
}

static size_t myreadfunc(SDL_RWops *context, void *ptr, size_t size, size_t maxnum)
{
    FSMgr::FileHandle *fil = (FSMgr::FileHandle *)context->hidden.unknown.data2;
    return fil->read(ptr, size * maxnum) / size;
}

static size_t mywritefunc(SDL_RWops *context, const void *ptr, size_t size, size_t num)
{
    FSMgr::FileHandle *fil = (FSMgr::FileHandle *)context->hidden.unknown.data2;
    return fil->write(ptr, size * num) / size;
}

static int myclosefunc(SDL_RWops *context)
{
    FSMgr::FileHandle *fil = (FSMgr::FileHandle *)context->hidden.unknown.data2;
    delete fil;

    SDL_FreeRW(context);
    return 0;
}

static SDL_RWops * MyCustomRWop(FSMgr::FileHandle *fil)
{
    SDL_RWops *c = SDL_AllocRW();

    if (c == NULL)
        return NULL;

    c->size = mysizefunc;
    c->seek = myseekfunc;
    c->read = myreadfunc;
    c->write = mywritefunc;
    c->close = myclosefunc;
    c->type = 0xF00BA91;
    c->hidden.unknown.data2 = fil;
    return c;
}




rsrc * NC_STACK_image::rsrc_func64(IDVList *stak)
{
    const char *resName = stak->GetConstChar(RSRC_ATT_NAME, NULL);
//    const char *reassignName = NULL;

    if ( !resName )
        return NULL;

    std::string tmpBuf = "rsrc:";
    tmpBuf += resName;

    FSMgr::FileHandle *fil = uaOpenFile(tmpBuf.c_str(), "rb");
    if (!fil)
        return NULL;

    SDL_RWops *rwops = MyCustomRWop(fil);
    if (!rwops)
    {
        delete fil;
        return NULL;
    }

    SDL_Surface *loaded = IMG_Load_RW(rwops, 1);

    if (!loaded)
    {
        printf("%s\n", SDL_GetError());
        return NULL;
    }

    IDVList loclist;
    if (!stak)
        stak = &loclist;

    stak->Add(BMD_ATT_WIDTH, loaded->w);
    stak->Add(BMD_ATT_HEIGHT, loaded->h);

    rsrc *res = NC_STACK_bitmap::rsrc_func64(stak); // bitmap_func64

    if ( !res )
    {
        SDL_FreeSurface(loaded);
        return NULL;
    }

    bitmap_intern *bitm = (bitmap_intern *)res->data;
    if ( !bitm )
    {
        SDL_FreeSurface(loaded);
        rsrc_func65(res);
        return NULL;
    }

    NC_STACK_win3d *win3d = GFXEngine::GFXe.getC3D();

    if (bitm->flags & BITMAP_FLAG_TEXTURE)
    {
        SDL_BlitSurface(loaded, NULL, bitm->swTex, NULL);

        if (!(bitm->flags & BITMAP_FLAG_SYSMEM))
            win3d->UpdateHwTexture(bitm);
    }
    else
    {
        if (loaded->format->BitsPerPixel > 8)
        {
            SDL_FreeSurface(loaded);
            rsrc_func65(res);
            return NULL;
        }

        if (loaded->format->palette)
        {
            if ( !bitm->pallete )
            {
                bitm->pallete = new UA_PALETTE;
                bitm->flags |= BITMAP_FLAG_HAS_PALETTE;
            }

            if ( bitm->pallete )
            {
                for (int i = 0; i < loaded->format->palette->ncolors; i++)
                    bitm->pallete->pal_entries[i] = loaded->format->palette->colors[i];
            }
        }

        SDL_LockSurface(loaded);
        uint8_t *dst = (uint8_t *)bitm->buffer;
        uint8_t *src = (uint8_t *)loaded->pixels;

        for(int i = 0; i < loaded->h; i++)
            memcpy( &dst[i * bitm->width], &src[i * loaded->pitch], bitm->width);

        SDL_UnlockSurface(loaded);
    }

    SDL_FreeSurface(loaded);

    return res;
}
