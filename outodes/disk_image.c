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

int read_int (int pos) {
  int *ptr = (int *) &rawdata[pos];
  return *ptr;
}

void allocate_dblocks(FILE *fpr, int nblocks, int indirect_blockno, int* nbytes) {
  for (int dblock_index = 0; dblock_index < BLOCK_SZ; dblock_index += sizeof(int)) {

    if (feof(fpr)) break;

    int direct_blockno = get_free_block(nblocks);
    write_int((indirect_blockno * BLOCK_SZ) + (dblock_index), direct_blockno);
    bitmap[direct_blockno] = 1;

    *nbytes += fread(rawdata + (direct_blockno * BLOCK_SZ), 1, BLOCK_SZ, fpr);
  }
}

void allocate_iblocks(FILE* fpr, int nblocks, int i2_blockno, int* nbytes) {
  for (int iblock_index = 0; iblock_index < BLOCK_SZ; iblock_index += sizeof(int)) {
    
    if (feof(fpr)) break;

    int indirect_blockno = get_free_block(nblocks);
    write_int((i2_blockno * BLOCK_SZ) + (iblock_index), indirect_blockno);
    bitmap[indirect_blockno] = 1;

    allocate_dblocks(fpr, nblocks, indirect_blockno, nbytes);
  }
}

void place_file(char *file, int uid, int gid, int nblocks, int inodepos, int block)
{
  int i, nbytes = 0;
  
  struct inode *ip = (struct inode *) (rawdata + (block * BLOCK_SZ) + (inodepos * sizeof(struct inode)));
  int magic_number = (block * BLOCK_SZ) + (inodepos * sizeof(struct inode));
  
  for (int ipidx = 0; ipidx < sizeof(struct inode); ipidx++) {
    if (rawdata[magic_number + ipidx]) {
      printf("Error: occupied inode \n");
      exit(-1);
    }
  }

  FILE *fpr;

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

    nbytes += fread(rawdata + (blockno * BLOCK_SZ), 1, BLOCK_SZ, fpr);
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
      write_int((i3_blockno * BLOCK_SZ) + (i2block_index), i2_blockno);
      bitmap[i2_blockno] = 1;

      allocate_iblocks(fpr, nblocks, i2_blockno, &nbytes);
    } 
  }

  ip->size = nbytes;  // total number of data bytes written for file
  printf("successfully wrote %d bytes of file %s\n", nbytes, file);

  if (fclose(fpr)) {
    perror("input file close");
    exit(-1);
  }
}

void check_dblocks(int iblock_no, int* remaining_blocks) {
  for (int dblock_index = 0; (dblock_index < BLOCK_SZ) && *remaining_blocks; dblock_index += sizeof(int)) {
    int direct_blockno = read_int((iblock_no * BLOCK_SZ) + dblock_index);
    bitmap[direct_blockno] = 1;
    (*remaining_blocks)--;
  }
}

void check_write_dblocks(int iblock_no, int* remaining_blocks, FILE* fptr) {
  for (int dblock_index = 0; (dblock_index < BLOCK_SZ) && *remaining_blocks; dblock_index += sizeof(int)) {
    int direct_blockno = read_int((iblock_no * BLOCK_SZ) + dblock_index);
    bitmap[direct_blockno] = 1;
    fwrite(&rawdata[direct_blockno * BLOCK_SZ], 1, BLOCK_SZ, fptr);
    (*remaining_blocks)--;
  }
}

void check_iblocks(int i2block_no, int* remaining_blocks) {
  for (int iblock_index = 0; (iblock_index < BLOCK_SZ) && *remaining_blocks; iblock_index += sizeof(int)) {
    int iblock_no = read_int((i2block_no * BLOCK_SZ) + iblock_index);
    bitmap[iblock_no] = 1;
    check_dblocks(iblock_no, remaining_blocks);
  }
}

void check_write_iblocks(int i2block_no, int* remaining_blocks, FILE* fptr) {
  for (int iblock_index = 0; (iblock_index < BLOCK_SZ) && *remaining_blocks; iblock_index += sizeof(int)) {
    int iblock_no = read_int((i2block_no * BLOCK_SZ) + iblock_index);

    bitmap[iblock_no] = 1;
    check_write_dblocks(iblock_no, remaining_blocks, fptr);
  }
}

void check_i2blocks(int i3block_no, int* remaining_blocks) {
  for (int i2block_index = 0; (i2block_index < BLOCK_SZ) && *remaining_blocks; i2block_index += sizeof(int)) {
    int i2block_no = read_int((i3block_no * BLOCK_SZ) + i2block_index);
    bitmap[i2block_no] = 1;
    check_iblocks(i2block_no, remaining_blocks);
  }
}

void check_write_i2blocks(int i3block_no, int* remaining_blocks, FILE* fptr) {
  for (int i2block_index = 0; (i2block_index < BLOCK_SZ) && *remaining_blocks; i2block_index += sizeof(int)) {
    int i2block_no = read_int((i3block_no * BLOCK_SZ) + i2block_index);
    bitmap[i2block_no] = 1;
    check_write_iblocks(i2block_no, remaining_blocks, fptr);
  }
}

void recreate_bitmap(int iblocks) {

  for (int block_index = 0; block_index < iblocks; block_index++) {
    // Looping throuh each ip in a block
    for (int ipidx = 0; (ipidx + sizeof(struct inode)) <= BLOCK_SZ; ipidx += sizeof(struct inode)) {
      
      struct inode *ip = (struct inode *) &rawdata[block_index * BLOCK_SZ + ipidx];

      
      if (ip->nlink == 0) continue;

      int remaining_blocks = ip->size % BLOCK_SZ == 0 ? ip->size / BLOCK_SZ : ip->size / BLOCK_SZ + 1;

      for (int dblock_index = 0; (dblock_index < N_DBLOCKS) && remaining_blocks; dblock_index++) {
        
        int dblock_no = ip->dblocks[dblock_index];
        bitmap[dblock_no] = 1;
        remaining_blocks--;
      }

      for (int iblock_index = 0; (iblock_index < N_IBLOCKS) && remaining_blocks; iblock_index++) {
        
        int iblock_no = ip->iblocks[iblock_index];
        bitmap[iblock_no] = 1;
        check_dblocks(iblock_no, &remaining_blocks);
      }

      if (remaining_blocks) {
        
        int i2block_no = ip->i2block;
        bitmap[i2block_no] = 1;
        check_iblocks(i2block_no, &remaining_blocks);

      }

      if (remaining_blocks) {

        int i3block_no = ip->i3block;
        bitmap[i3block_no] = 1;
        check_i2blocks(i3block_no, &remaining_blocks);

      }
    }
  }

}

int extract_files(int nblocks, int uid, int gid, char* path) {
  
  int files_found = 0;
  
  for (int block_index = 0; block_index < nblocks; block_index++) {
    for (int ipidx = 0; (ipidx + sizeof(struct inode)) <= BLOCK_SZ; ipidx += sizeof(struct inode)) {
      
      struct inode *ip = (struct inode *) &rawdata[block_index * BLOCK_SZ + ipidx];

      if (!(ip->uid == uid && ip->gid == gid)) {
        continue;
      }

      // Has valid inode
      int remaining_blocks = ip->size % BLOCK_SZ == 0 ? ip->size / BLOCK_SZ : ip->size / BLOCK_SZ + 1;
      bitmap[block_index] = 1;

      files_found++;

      printf("file found at inode in block %d, file size %d\n", block_index, ip->size);

      char filename[100];
      sprintf(filename, "%s/%d", path, files_found);

      FILE* fptr = fopen(filename, "wb"); // CLOSE ME <- didit :p
      if (!fptr) {
        perror("output file open");
        exit(-1);
      }


      for (int dblock_index = 0; (dblock_index < N_DBLOCKS) && remaining_blocks; dblock_index++) {

        int dblock_no = ip->dblocks[dblock_index];
        
        bitmap[dblock_no] = 1;
        fwrite(&rawdata[dblock_no * BLOCK_SZ], 1, BLOCK_SZ, fptr);
        remaining_blocks--;
      }

      for (int iblock_index = 0; (iblock_index < N_IBLOCKS) && remaining_blocks; iblock_index++) {
        
        int iblock_no = ip->iblocks[iblock_index];
        
        bitmap[iblock_no] = 1;
        check_write_dblocks(iblock_no, &remaining_blocks, fptr);
      }

      
      if (remaining_blocks) {
        
        int i2block_no = ip->i2block;
        
        bitmap[i2block_no] = 1;
        check_write_iblocks(i2block_no, &remaining_blocks, fptr);
      }

      if (remaining_blocks) {
      
        int i3block_no = ip->i3block;
        
        bitmap[i3block_no] = 1;
        check_write_i2blocks(i3block_no, &remaining_blocks, fptr); 
      }

      truncate(filename, ip->size);
      if(fclose(fptr)) {
        perror("schlonk");
      }
    }
  }

  return files_found;

}

int main(int argc, char* argv[]) // add argument handling
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
    assert(nblocks <= TOTAL_BLOCKS);
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
    return 0;
  } else if (insert) {
    int i;
    FILE *outfile;

    assert(block < iblocks);
    assert(iblocks < nblocks);
    assert(nblocks <= TOTAL_BLOCKS);
    assert((inodepos * sizeof(struct inode) + sizeof(struct inode)) <= BLOCK_SZ);
    
    memset(rawdata, 0, nblocks*BLOCK_SZ);
    memset(bitmap, 1, iblocks);
    memset(bitmap + iblocks, 0, nblocks - iblocks);
    
    outfile = fopen(imagefile, "rb+"); // look at me
    if (!outfile) {
      perror("output file open");
      exit(-1);
    }

    fread(rawdata, 1, nblocks*BLOCK_SZ, outfile);

    recreate_bitmap(iblocks);

    // fill in here to place file 
    place_file(inputfile, uid, gid, nblocks, inodepos, block);

    rewind(outfile);
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
    return 0;
  } else if (extract) {
    // Loop through each inode in each block
    // Check the uid and gid fields of each of these ip
    
    // extract_files()

    int i;
    FILE *infile;
    
    memset(rawdata, 0, TOTAL_BLOCKS*BLOCK_SZ);
    memset(bitmap, 0, TOTAL_BLOCKS);
    
    infile = fopen(imagefile, "rb+");
    if (!infile) {
      perror("output file open");
      exit(-1);
    }

    int nbytes_read = fread(rawdata, 1, TOTAL_BLOCKS*BLOCK_SZ, infile);
    nblocks = nbytes_read % BLOCK_SZ == 0 ? nbytes_read / BLOCK_SZ : nbytes_read / BLOCK_SZ + 1;

    extract_files(nblocks, uid, gid, outputpath);

    i = fclose(infile);
    if (i) {
      perror("infile file close");
      exit(-1);
    }

    FILE *unused;
    char unused_filename[120];

    sprintf(unused_filename, "%s/UNUSED_BLOCKS", outputpath);

    unused = fopen(unused_filename, "w");
    if (!unused) {
      perror("problem opening file for unused blocks");
      exit(-1);
    }

    for (int bitmap_index = 0; bitmap_index < nblocks; bitmap_index++) {
      if (!bitmap[bitmap_index]) {
          fprintf(unused, "%d\n", bitmap_index);
      }
    }

    if (fclose(unused)) {
      perror("problem closing unused blocks file");
      exit(-1);
    }

    printf("Done.\n");

  }

  return 0;
}
