// ============================================================
//  AKASH-DARPAN  —  tools/gen_synthetic_data.cpp
//  Standalone tool to generate a .bmp SH-WFS time-series
//  for testing AKASH-DARPAN without real hardware data.
//  Compile: g++ -O2 -o gen_data gen_synthetic_data.cpp
// ============================================================
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct BmpHeader {
    uint8_t  sig[2]    = {'B','M'};
    uint32_t fileSize  = 0;
    uint32_t reserved  = 0;
    uint32_t dataOffset= 54 + 1024; // + palette for 8bpp
    uint32_t headerSize= 40;
    int32_t  width     = 0;
    int32_t  height    = 0;
    uint16_t planes    = 1;
    uint16_t bpp       = 8;
    uint32_t compress  = 0;
    uint32_t imgSize   = 0;
    int32_t  xPPM      = 2835;
    int32_t  yPPM      = 2835;
    uint32_t clrUsed   = 256;
    uint32_t clrImportant = 256;
};

static void writeBmp(const std::string& path,
                     const std::vector<uint8_t>& pixels,
                     int W, int H) {
    BmpHeader hdr;
    hdr.width    = W;
    hdr.height   = -H; // top-down
    int rowBytes = (W + 3) & ~3;
    hdr.imgSize  = rowBytes * H;
    hdr.fileSize = 54 + 1024 + hdr.imgSize;

    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "Cannot write %s\n", path.c_str()); return; }

    // BMP signature
    fwrite("BM", 1, 2, f);
    fwrite(&hdr.fileSize, 4, 1, f);
    uint32_t res=0; fwrite(&res,4,1,f);
    fwrite(&hdr.dataOffset,4,1,f);
    fwrite(&hdr.headerSize,4,1,f);
    fwrite(&hdr.width,4,1,f);
    fwrite(&hdr.height,4,1,f);
    fwrite(&hdr.planes,2,1,f);
    fwrite(&hdr.bpp,2,1,f);
    fwrite(&hdr.compress,4,1,f);
    fwrite(&hdr.imgSize,4,1,f);
    fwrite(&hdr.xPPM,4,1,f);
    fwrite(&hdr.yPPM,4,1,f);
    fwrite(&hdr.clrUsed,4,1,f);
    fwrite(&hdr.clrImportant,4,1,f);

    // Greyscale palette
    for (int i = 0; i < 256; ++i) {
        uint8_t p[4] = {(uint8_t)i,(uint8_t)i,(uint8_t)i,0};
        fwrite(p,1,4,f);
    }

    // Pixel data (padded rows)
    std::vector<uint8_t> row(rowBytes, 0);
    for (int y = 0; y < H; ++y) {
        memcpy(row.data(), pixels.data() + y*W, W);
        fwrite(row.data(), 1, rowBytes, f);
    }
    fclose(f);
}

static double turbPhase(double x, double y, double r0, double t, int s) {
    double phase = 0;
    double D = 0.15;
    double amp = std::pow(D/r0, 5.0/6.0) * 0.5;
    for (int k = 1; k <= 8; ++k) {
        double kx = k*(s%3+1)*2.0, ky = k*((s+1)%3+1)*1.7;
        double phi = s*k*0.31;
        phase += amp/k * std::sin(kx*x + ky*y + phi + t*0.8/k);
    }
    return phase;
}

int main(int argc, char* argv[]) {
    int   nFrames   = 100;
    int   W = 512, H = 512;
    int   nx = 10,  ny = 10;
    double r0 = 0.12;
    const char* outDir = "./synthetic_data";

    if (argc > 1) outDir   = argv[1];
    if (argc > 2) nFrames  = atoi(argv[2]);
    if (argc > 3) r0       = atof(argv[3]);

    fs::create_directories(outDir);
    printf("[GenData] Generating %d SH-WFS frames → %s  (r0=%.3f m)\n",
           nFrames, outDir, r0);

    int cellW = W/nx, cellH = H/ny;
    double sigma = cellW * 0.12;

    for (int f = 0; f < nFrames; ++f) {
        double t = f * 10e-3; // 10 ms cadence
        std::vector<uint8_t> img(W*H, 4);

        for (int iy = 0; iy < ny; ++iy) {
            for (int ix = 0; ix < nx; ++ix) {
                double cx = (ix+0.5)/nx*2-1;
                double cy = (iy+0.5)/ny*2-1;
                if (std::sqrt(cx*cx+cy*cy) > 0.95) continue;

                double refX = (ix+0.5)*cellW, refY = (iy+0.5)*cellH;
                double dp_dx = turbPhase(cx+0.01,cy,r0,t,ix*13+iy)
                             - turbPhase(cx-0.01,cy,r0,t,ix*13+iy);
                double dp_dy = turbPhase(cx,cy+0.01,r0,t,ix*13+iy)
                             - turbPhase(cx,cy-0.01,r0,t,ix*13+iy);
                double shX = dp_dx/0.02 * cellW*0.25;
                double shY = dp_dy/0.02 * cellH*0.25;

                double spotX = refX + shX, spotY = refY + shY;
                int x0=std::max(0,(int)(spotX-3*sigma)), x1=std::min(W-1,(int)(spotX+3*sigma));
                int y0=std::max(0,(int)(spotY-3*sigma)), y1=std::min(H-1,(int)(spotY+3*sigma));
                for (int py=y0;py<=y1;++py)
                    for (int px=x0;px<=x1;++px) {
                        double dx=px-spotX, dy=py-spotY;
                        double v=235*std::exp(-(dx*dx+dy*dy)/(2*sigma*sigma));
                        int cur=img[py*W+px];
                        img[py*W+px]=(uint8_t)std::min(255,cur+(int)v);
                    }
            }
        }

        char fname[256];
        snprintf(fname, sizeof(fname), "%s/frame_%04d.bmp", outDir, f);
        writeBmp(fname, img, W, H);

        if ((f+1) % 10 == 0)
            printf("  %d/%d frames written\r", f+1, nFrames);
    }
    printf("\n[GenData] Done.  Load with: ./akash_darpan %s\n", outDir);
    return 0;
}
