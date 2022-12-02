#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "inode.h"

#define TOTAL_BLOCKS (10*1024)

static unsigned char rawdata[TOTAL_BLOCKS*BLOCK_SZ];
static char bitmap[TOTAL_BLOCKS];

int get_free_block(int nblocks)
{
  int blockno;
  for (blockno = 0; blockno < nblocks; blockno++) {
    if (!bitmap[blockno]) break;
  }
  assert(blockno < nblocks);
  assert(!bitmap[blockno]);
  return blockno;
}

void write_int(int pos, int val)
{
  int *ptr = (int *)&rawdata[pos];
  *ptr = val;
}

void allocate_dblocks(FILE *fpr, int nblocks, int indirect_blockno, int* nbytes) {
  for (int dblock_index = 0; dblock_index < BLOCK_SZ; dblock_index += sizeof(int)) {

    if (feof(fpr)) break;

    int direct_blockno = get_free_block(nblocks);
    write_int((indirect_blockno * BLOCK_SZ) + (dblock_index * sizeof(int)), direct_blockno);
    bitmap[direct_blockno] = 1;

    fread(rawdata + (direct_blockno * BLOCK_SZ), 1, BLOCK_SZ, fpr);
    *nbytes += BLOCK_SZ;
  }
}

void allocate_iblocks(FILE* fpr, int nblocks, int i2_blockno, int* nbytes) {
  for (int iblock_index = 0; iblock_index < BLOCK_SZ; iblock_index += sizeof(int)) {
    
    if (feof(fpr)) break;

    int indirect_blockno = get_free_block(nblocks);
    write_int((i2_blockno * BLOCK_SZ) + (iblock_index * sizeof(int)), indirect_blockno);
    bitmap[indirect_blockno] = 1;

    allocate_dblocks(fpr, nblocks, indirect_blockno, nbytes);
  }
}

void place_file(char *file, int uid, int gid, int nblocks, int inodepos, int block)
{
  int i, nbytes = 0;
  int i2block_index, i3block_index;
  struct inode *ip;
  FILE *fpr;
  unsigned char buf[BLOCK_SZ];

  ip->mode = 0;
  ip->nlink = 1;
  ip->uid = uid;
  ip->gid = gid;
  ip->ctime = 0;
  ip->mtime = 0;
  ip->atime = 0;

  fpr = fopen(file, "rb");
  if (!fpr) {
    perror(file);
    exit(-1);
  }

  for (i = 0; i < N_DBLOCKS; i++) {
    
    if (feof(fpr)) break;

    int blockno = get_free_block(nblocks);
    ip->dblocks[i] = blockno;
    bitmap[blockno] = 1;

    fread(rawdata + (blockno * BLOCK_SZ), 1, BLOCK_SZ, fpr);
    nbytes += BLOCK_SZ;
  }


  // fill in here if IBLOCKS needed
  // if so, you will first need to get an empty block to use for your IBLOCK
  if (!feof(fpr)) {
    for (int iblock_index = 0; iblock_index < N_IBLOCKS; iblock_index++) {
      
      if (feof(fpr)) break;

      int indirect_blockno = get_free_block(nblocks);
      ip->iblocks[iblock_index] = indirect_blockno;
      bitmap[indirect_blockno] = 1;

      allocate_dblocks(fpr, nblocks, indirect_blockno, &nbytes);
    }
  }

  if (!feof(fpr)) {
    int i2_blockno = get_free_block(nblocks);
    ip->i2block = i2_blockno;
    bitmap[i2_blockno] = 1;

    allocate_iblocks(fpr, nblocks, i2_blockno, &nbytes);
  }

  if (!feof(fpr)) {
    int i3_blockno = get_free_block(nblocks);
    ip->i3block = i3_blockno;
    bitmap[i3_blockno] = 1;   

    for (int i2block_index = 0; i2block_index < BLOCK_SZ; i2block_index += sizeof(int)) {

      if (feof(fpr)) break;

      int i2_blockno = get_free_block(nblocks);
      write_int((i3_blockno * BLOCK_SZ) + (i2block_index * sizeof(int)), i2_blockno);
      bitmap[i2_blockno] = 1;

      allocate_iblocks(fpr, nblocks, i2_blockno, &nbytes);
    } 
  }

  ip->size = nbytes;  // total number of data bytes written for file
  printf("successfully wrote %d bytes of file %s\n", nbytes, file);

  memcpy(rawdata + (block * BLOCK_SZ) + (inodepos * sizeof(struct inode)), ip, sizeof(struct inode));

  if (fclose(fpr)) {
    perror("input file close");
    exit(-1);
  }
}

void main(int argc, char* argv[]) // add argument handling
{
  int create = 0;
  int insert = 0;
  int extract = 0;

  enum input_args {
    IMAGE = 1,
    NBLOCKS,
    IBLOCKS,
    INPUTFILE,
    UID,
    GID,
    BLOCK,
    INODEPOS,
    O,
  };

  int nblocks, iblocks, uid, gid, block, inodepos;
  char inputfile[100], imagefile[100], outputpath[100];

  struct option long_options[] = {
    {"create",    no_argument,       &create,  1          },
    {"insert",    no_argument,       &insert,  1          },
    {"extract",   no_argument,       &extract, 1          },
    {"image",     required_argument, 0,        IMAGE      },
    {"nblocks",   required_argument, 0,        NBLOCKS    },
    {"iblocks",   required_argument, 0,        IBLOCKS    },
    {"inputfile", required_argument, 0,        INPUTFILE  },
    {"u",         required_argument, 0,        UID        },
    {"g",         required_argument, 0,        GID        },
    {"block",     required_argument, 0,        BLOCK      },
    {"inodepos",  required_argument, 0,        INODEPOS   },
    {"o",         required_argument, 0,        O          },
    {0,           0,                 0,        0          },

  };

  int c;

  while(1) {
    int option_index = 0;
    c = getopt_long_only(argc, argv, "", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch(c) {
      case 0:
        break;
      case IMAGE:
        sscanf(optarg, "%s", imagefile);
        break;
      case NBLOCKS:
        sscanf(optarg, "%d", &nblocks);
        break;   
      case IBLOCKS:
        sscanf(optarg, "%d", &iblocks);
        break;
      case INPUTFILE:
        sscanf(optarg, "%s", inputfile);
        break;
      case UID:
        sscanf(optarg, "%d", &uid);
        break;
      case GID:
        sscanf(optarg, "%d", &gid);
        break;
      case BLOCK:
        sscanf(optarg, "%d", &block);
        break;
      case INODEPOS:
        sscanf(optarg, "%d", &inodepos);
        break;
      case O:
        sscanf(optarg, "%s", outputpath);
        break;   
      default:
        printf("womp womp\n");
        break;     
    }
  }

  if (create) {
    int i;
    FILE *outfile;

    assert(block < iblocks);
    assert(iblocks < nblocks);
    assert((inodepos * sizeof(struct inode) + sizeof(struct inode)) <= BLOCK_SZ);
    
    memset(rawdata, 0, nblocks*BLOCK_SZ);
    memset(bitmap, 1, iblocks);
    memset(bitmap + iblocks, 0, nblocks - iblocks);
    
    outfile = fopen(imagefile, "wb");
    if (!outfile) {
      perror("output file open");
      exit(-1);
    }

    // fill in here to place file 
    place_file(inputfile, uid, gid, nblocks, inodepos, block);

    i = fwrite(rawdata, 1, nblocks*BLOCK_SZ, outfile);
    if (i != nblocks*BLOCK_SZ) {
      perror("fwrite");
      exit(-1);
    }

    i = fclose(outfile);
    if (i) {
      perror("output file close");
      exit(-1);
    }

    printf("Done.\n");
    return;
  } else if (insert) {

  } else if (extract) {

  }
}
