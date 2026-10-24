/* FreeType uses bounded memory faces. Only a baked ASCII atlas survives load. */
#include "os.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
extern void FT_Add_Default_Modules(FT_Library);
static void*ft_alloc(FT_Memory m,long n){return n>0?alloc(n):0;}
static void ft_free(FT_Memory m,void*p){dealloc(p);}
static void*ft_realloc(FT_Memory m,long old,long size,void*p){if(size<=0){dealloc(p);return 0;}void*q=alloc(size);if(q){if(p)memcpy(q,p,old<size?old:size);dealloc(p);}return q;}
FT_Memory FT_New_Memory(void){FT_Memory m=alloc(sizeof(*m));if(m){m->user=0;m->alloc=ft_alloc;m->free=ft_free;m->realloc=ft_realloc;}return m;}
void FT_Done_Memory(FT_Memory m){dealloc(m);}
int truetype_load(const char*path,u8 atlas[128][96]){
    int f=fs_find(path),n=fs_size(f),result=-1;if(n<12||n>2*1024*1024)return -1;
    u8*bytes=alloc(n);if(!bytes)return -1;if(fs_read(f,0,bytes,n)!=n){dealloc(bytes);return -1;}
    struct FT_MemoryRec_ memory={0,ft_alloc,ft_free,ft_realloc};FT_Library library=0;FT_Face face=0;
    if(FT_New_Library(&memory,&library))goto done;FT_Add_Default_Modules(library);
    if(FT_New_Memory_Face(library,bytes,n,0,&face)||FT_Select_Charmap(face,FT_ENCODING_UNICODE)||FT_Set_Pixel_Sizes(face,0,11))goto done;
    memset(atlas,0,128*96);
    for(int ch=32;ch<127;ch++){
        if(FT_Load_Char(face,ch,FT_LOAD_RENDER|FT_LOAD_NO_HINTING))goto done;
        FT_GlyphSlot g=face->glyph;FT_Bitmap*b=&g->bitmap;if(b->pixel_mode!=FT_PIXEL_MODE_GRAY&&b->rows)goto done;
        int width=b->width>8?8:b->width,left=g->bitmap_left;if(left<0)left=0;if(left+width>8)left=8-width;
        for(int y=0;y<(int)b->rows;y++){int dy=10-g->bitmap_top+y;if(dy<0||dy>=12)continue;
            const u8*row=b->buffer+(b->pitch<0?(b->rows-1-y)*-b->pitch:y*b->pitch);
            for(int x=0;x<width;x++)atlas[ch][dy*8+left+x]=row[x*b->width/width];}
    }
    result=0;serial("FONT: TrueType rasterized\n");
done:if(face)FT_Done_Face(face);if(library)FT_Done_Library(library);dealloc(bytes);return result;
}
