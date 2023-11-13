#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "eprintf.h"
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define ONE_MEG 1024*1024
static GChecksum *Checksum;

void print_string(gpointer item,gpointer data) {
  printf("%s\n",(char *
  )item);
}

void process_file(const gpointer item,gpointer hash) {
  gsize pos,len;
  guchar *buffer;
  char *sha;
  GError *error = NULL;
  GMappedFile *mfile;
  GPtrArray *filelist=NULL;

  g_checksum_reset(Checksum);
  mfile = g_mapped_file_new(item,FALSE,&error);
  if (error == NULL) {
    len = g_mapped_file_get_length(mfile);
    buffer = (guchar *)g_mapped_file_get_contents(mfile);
    for (pos=0;pos<len;pos+=ONE_MEG) {
      g_checksum_update(Checksum,buffer,min(ONE_MEG,len-pos));
    }

    sha=strdup(g_checksum_get_string(Checksum));
    //printf("File %s %s\n",(char *)item,sha);
    filelist = g_hash_table_lookup(hash,sha);
    if (filelist == NULL) {
      filelist = g_ptr_array_new_full(4,g_free);
      g_hash_table_insert(hash,sha,filelist);
    }
    else
      free(sha);
    g_ptr_array_add(filelist,strdup(item));
    g_mapped_file_unref(mfile);
  }
  else {
    eprintf("Error:%s\n",error->message);
    g_error_free(error);
  }
}

void process_path(gpointer item, gpointer hash);
void process_dir(gpointer item, gpointer hash) {
  GDir *dir;
  GError *error=NULL;
  const gchar *filename;
  char *file;

  //eprintf("process_dir: %s\n",(char *)item);
  dir = g_dir_open(item, 0, &error);
  if (error == NULL) {
    while ((filename = g_dir_read_name(dir))) {
      file = g_build_path("/",item,filename,NULL);
      process_path(file,hash);
      g_free(file);
    }
    g_dir_close(dir);
  }
  else {
    eprintf("Error listing directory %s\n",(char *)item);
    g_error_free(error);
  }
}

void process_path(gpointer item, gpointer hash) {
  //eprintf("process_path: %s\n",(char *)item);
  if (g_file_test(item,G_FILE_TEST_IS_DIR))
    process_dir(item,hash);
  if (g_file_test(item,G_FILE_TEST_IS_REGULAR))
    process_file(item,hash);
}

void free_string(gpointer item) {
  printf("%s: %s\n",__FUNCTION__,(char *)item);
  free(item);
}

void free_string_(gpointer item, gpointer data) {
  printf("%s: %s\n",__FUNCTION__,(char *)item);
  free(item);
}

void free_slist_and_strings(gpointer item) {
  g_slist_foreach(item,free_string_,NULL);
  g_slist_free(item);
}

void free_ptr_array(gpointer item) {
  g_ptr_array_unref(item);
}

void show_duplicates(gpointer key, gpointer value, gpointer dummy) {
  guint i;
  GPtrArray *list = value;

  //printf("%s %d\n",(char *)key,list->len);
  if (list->len > 1) {
    for (i=0;i<list->len;i++)
      printf("%d %d %s\n",list->len,i+1,(char *)list->pdata[i]);
  }
}

int main(int argc, char** argv) {
  GSList *paths=NULL;
  GHashTable *hash;
  int i;

  Checksum = g_checksum_new(G_CHECKSUM_SHA256);
  // setup hastable of Arrays
  hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_ptr_array);

  // setup paths from the command line
  if (argc == 1)
    paths = g_slist_append(paths,".");
  else
    for (i=1;i<argc;i++)
      paths = g_slist_append(paths,argv[i]);

  //walk any paths or directories
  g_slist_foreach(paths,process_path,hash);

  // display duplicates
  g_hash_table_foreach(hash,show_duplicates,NULL);

  //printf("Finish\n");

  //finish
  g_checksum_free(Checksum);
  g_hash_table_remove_all(hash);
  g_hash_table_unref(hash);
  g_slist_free(paths);
}
