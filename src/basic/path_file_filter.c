#include "global.h"
#include "path_file_filter.h"
#include "path_format2.h"
#include "util.h"

const PathFileHeader INIT_PATH_FILE_HDR = INIT_PATH_FILE_HDR_MACRO;
const PathFileReader INIT_PATH_READER = INIT_PATH_READER_MACRO;

int path_file_open(PathFileReader *file, char *path, boolean fatal)
{
  return path_file_open2(file, path, fatal, "r");
}

// Open file
// if cannot open file returns 0
// if fatal is true, exits on error
// if !fatal, returns -1 on error
// if successful creates a new PathFileReader and returns 1
int path_file_open2(PathFileReader *file, char *path, boolean fatal,
                    const char *mode)
{
  PathFileHeader *hdr = &file->hdr;
  FileFilter *fltr = &file->fltr;

  if(!file_filter_alloc(fltr, path, mode, fatal)) return 0;
  setvbuf(fltr->fh, NULL, _IOFBF, CTP_BUF_SIZE);

  file->hdr_size = paths2_file_read_header(fltr->fh, hdr, fatal, fltr->path.buff);
  if(file->hdr_size == -1) return -1;

  file_filter_set_cols(fltr, hdr->num_of_cols);

  // Check we can handle the kmer size
  file_filter_check_kmer_size(file->hdr.kmer_size, file->fltr.path.buff);

  // Check file length
  off_t file_len = file->hdr_size + hdr->num_path_bytes +
                   hdr->num_kmers_with_paths *
                   (NUM_BKMER_WORDS*sizeof(uint64_t)+sizeof(uint64_t));

  if(file_len != file->fltr.file_size) {
    warn("Corrupted file? Sizes don't match up "
         "[hdr:%zu exp:%zu actual:%zu path: %s]",
         (size_t)file->hdr_size, (size_t)file_len,
         (size_t)file->fltr.file_size, file->fltr.path.buff);
  }

  return 1;
}

// File loading checks against a given graph
void path_file_load_check(const PathFileReader *file, const dBGraph *db_graph)
{
  const FileFilter *fltr = &file->fltr;
  const PathFileHeader *hdr = &file->hdr;

  if(file->hdr.kmer_size != db_graph->kmer_size) {
    die("Kmer sizes do not match between graph and path file [%s]",
        fltr->orig.buff);
  }

  if(path_file_usedcols(file) > db_graph->num_of_cols) {
    die("Number of colours in path file is greater than in the graph [%s]",
        fltr->orig.buff);
  }

  if(hdr->num_path_bytes > db_graph->pdata.size) {
    char mem_str[100]; bytes_to_str(hdr->num_path_bytes, 1, mem_str);
    die("Not enough memory allocated to store paths [mem: %s path: %s]",
        mem_str, fltr->orig.buff);
  }

  if(db_graph->ht.unique_kmers > 0 &&
     db_graph->ht.unique_kmers < hdr->num_kmers_with_paths)
  {
    warn("Graph has fewer kmers than paths file");
  }

  // Check sample names match
  size_t i, col;
  for(i = 0; i < hdr->num_of_cols; i++)
  {
    col = file_filter_intocol(fltr, i);
    char *gname = db_graph->ginfo[col].sample_name.buff;
    char *pname = hdr->sample_names[i].buff;

    if(strcmp(pname, "noname") != 0 && strcmp(gname, pname) != 0) {
      die("Graph/path sample names do not match [%zu->%zu] '%s' vs '%s'",
          i, col, gname, pname);
    }
  }
}

// Close file
void path_file_close(PathFileReader *file)
{
  file_filter_close(&file->fltr);
}

// calls file_filter_dealloc which will close file if needed
void path_file_dealloc(PathFileReader *file)
{
  file_filter_dealloc(&file->fltr);
  paths2_header_dealloc(&file->hdr);
}