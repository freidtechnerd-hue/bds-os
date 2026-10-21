/* Parse only checksummed, bounded firmware monitor descriptions. */
static int edid_checksum(const unsigned char*b){unsigned sum=0;for(int i=0;i<128;i++)sum+=b[i];return !(sum&255);}
static int edid_base(const unsigned char*b){
    static const unsigned char magic[8]={0,255,255,255,255,255,255,0};
    for(int i=0;i<8;i++)if(b[i]!=magic[i])return -1;
    return edid_checksum(b)&&b[18]==1&&b[19]<=4?0:-1;
}
static int edid_size(int x,int y,int*w,int*h){if(x<640||x>3840||y<480||y>2160)return -1;*w=x;*h=y;return 0;}
static int edid_preferred(const unsigned char*b,int*w,int*h){
    if(edid_base(b))return -1;
    const unsigned char*d=b+54;if(!(b[24]&2)||!(d[0]|d[1])||(d[17]&128))return -1;
    return edid_size(d[2]+((d[4]&240)<<4),d[5]+((d[7]&240)<<4),w,h);
}
static int edid_extension(const unsigned char*b,int*w,int*h){
    if(!edid_checksum(b))return -1;
    if(b[0]==0x70&&b[1]==0x13){
        int end=5+b[2];if(end>126)return -1;unsigned sum=0;
        for(int i=1;i<=end;i++)sum+=b[i];if(sum&255)return -1;
        for(int p=5;p+3<=end;){int n=b[p+2];if(p+3+n>end)return -1;
            if(b[p]==3&&!(n%20))for(int j=p+3;j+20<=p+3+n;j+=20){const unsigned char*d=b+j;
                if((d[3]&0x90)==0x80&&!edid_size(1+d[4]+256*d[5],1+d[12]+256*d[13],w,h))return 0;
            }p+=3+n;
        }
    }
    /* CTA native detailed timings. Non-native video modes are not a monitor preference. */
    if(b[0]==2&&b[1]>=3&&b[2]>=4&&b[2]<=109){int p=b[2],n=b[3]&15;
        for(int i=0;i<n&&p+18<=127;i++,p+=18){const unsigned char*d=b+p;
            if((d[0]|d[1])&&!(d[17]&128)&&!edid_size(d[2]+((d[4]&240)<<4),d[5]+((d[7]&240)<<4),w,h))return 0;
        }
    }return -1;
}
