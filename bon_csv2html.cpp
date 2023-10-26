#include "bonnie.h"
#include <stdio.h>
#include <vector>
#include <string.h>
#include <math.h>

// Maximum number of items expected on a csv line
#define MAX_ITEMS 48
#ifndef OS2
using namespace std;
#endif
typedef vector<PCCHAR> STR_VEC;

#ifndef HAVE_MIN_MAX
#if defined(HAVE_ALGO_H) || defined(HAVE_ALGO)
#ifdef HAVE_ALGO
#include <algo>
#else
#include <algo.h>
#endif
#else
#define min(XX,YY) ((XX) < (YY) ? (XX) : (YY))
#define max(XX,YY) ((XX) > (YY) ? (XX) : (YY))
#endif
#endif

vector<STR_VEC> data;
typedef PCCHAR * PPCCHAR;
PPCCHAR * props;

// Print the start of the HTML file
// return the number of columns space in the middle
int header();
// Splits a line of text (CSV format) by commas and adds it to the list to
// process later.  Doesn't keep any pointers to the buf...
void read_in(CPCCHAR buf);
// print line in the specified line from columns start..end as a line of a
// HTML table
void print_a_line(int num, int start, int end);
// print a single item of data
void print_item(int num, int item);
// Print the end of the HTML file
void footer();
// Calculate the colors for backgrounds
void calc_vals();
// Returns a string representation of a color that maps to the value.  The
// range of values is 0..range_col and val is the value.  If reverse is set
// then low values are green and high values are red.
PCCHAR get_col(double range_col, double val, bool reverse, CPCCHAR extra);

typedef enum { eNoCols, eSpeed, eCPU, eLatency } VALS_TYPE;
const VALS_TYPE vals[MAX_ITEMS] =
  { eNoCols,eNoCols,eNoCols,eNoCols,eNoCols,eNoCols,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,
    eNoCols,eNoCols,eNoCols,eNoCols,eNoCols,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,eSpeed,eCPU,
    eLatency,eLatency,eLatency,eLatency,eLatency,eLatency,eLatency,eLatency,eLatency,eLatency,eLatency,eLatency };

bool col_used[MAX_ITEMS];
#define COL_NAME 2
#define COL_CONCURRENCY 3
#define COL_FILE_SIZE 5
#define COL_DATA_CHUNK_SIZE 6
#define COL_PUTC 7
#define COL_NUM_FILES 19
#define COL_MAX_SIZE 20
#define COL_MIN_SIZE 21
#define COL_NUM_DIRS 22
#define COL_FILE_CHUNK_SIZE 23
#define COL_RAN_DEL_CPU 35
#define COL_PUTC_LATENCY 36
#define COL_SEEKS_LATENCY 41
#define COL_SEQ_CREATE_LATENCY 42
#define COL_RAN_DEL_LATENCY 47

void usage()
{
  exit(1);
}

int main(int argc, char **argv)
{
  unsigned int i;
  for(i = 0; i < MAX_ITEMS; i++)
    col_used[i] = false;

  char buf[1024];

  FILE *fp = NULL;
  if(argc > 1)
  {
    fp = fopen(argv[1], "r");
    if(!fp)
      usage();
  }
  while(fgets(buf, sizeof(buf), fp ? fp : stdin))
  {
    buf[sizeof(buf) - 1] = '\0';
    strtok(buf, "\r\n");
    read_in(buf);
  }

  props = new PPCCHAR[data.size()];
  for(i = 0; i < data.size(); i++)
  {
    props[i] = new PCCHAR[MAX_ITEMS];
    props[i][0] = NULL;
    props[i][1] = NULL;
    props[i][COL_NAME] = "bgcolor=\"#FFFFFF\" class=\"rowheader\"><FONT SIZE=+1";
    int j;
    for(j = COL_CONCURRENCY; j < MAX_ITEMS; j++)
    {
      if( (j >= COL_NUM_FILES && j <= COL_FILE_CHUNK_SIZE) || j <= COL_DATA_CHUNK_SIZE )
      {
        props[i][j] = "class=\"size\" bgcolor=\"#FFFFFF\"";
      }
      else
      {
        props[i][j] = NULL;
      }
    }
  }
  calc_vals();
  int mid_width = header();
  for(i = 0; i < data.size(); i++)
  {
// First print the average speed line
    printf("<TR>");
    print_item(i, COL_NAME);
    if(col_used[COL_CONCURRENCY] == true)
      print_item(i, COL_CONCURRENCY);
    print_item(i, COL_FILE_SIZE); // file_size
    if(col_used[COL_DATA_CHUNK_SIZE] == true)
      print_item(i, COL_DATA_CHUNK_SIZE);
    print_a_line(i, COL_PUTC, COL_NUM_FILES);
    if(col_used[COL_MAX_SIZE])
      print_item(i, COL_MAX_SIZE);
    if(col_used[COL_MIN_SIZE])
      print_item(i, COL_MIN_SIZE);
    if(col_used[COL_NUM_DIRS])
      print_item(i, COL_NUM_DIRS);
    if(col_used[COL_FILE_CHUNK_SIZE])
      print_item(i, COL_FILE_CHUNK_SIZE);
    print_a_line(i, COL_FILE_CHUNK_SIZE + 1, COL_RAN_DEL_CPU);
    printf("</TR>\n");
// Now print the latency line
    printf("<TR>");
    print_item(i, COL_NAME);
    int lat_width = 1;
    if(col_used[COL_DATA_CHUNK_SIZE] == true)
      lat_width++;
    if(col_used[COL_CONCURRENCY] == true)
      lat_width++;
    printf("<TD class=\"size\" bgcolor=\"#FFFFFF\" COLSPAN=%d>Latency</TD>"
         , lat_width);
    print_a_line(i, COL_PUTC_LATENCY, COL_SEEKS_LATENCY);
    int bef_lat_width;
    lat_width = 1;
    if(mid_width > 1)
      lat_width = 2;
    bef_lat_width = mid_width - lat_width;
    if(bef_lat_width)
      printf("<TD COLSPAN=%d></TD>", bef_lat_width);
    printf("<TD class=\"size\" bgcolor=\"#FFFFFF\" COLSPAN=%d>Latency</TD>", lat_width);
    print_a_line(i, COL_SEQ_CREATE_LATENCY, COL_RAN_DEL_LATENCY);
    printf("</TR>\n");
  }
  footer();
  return 0;
}

typedef struct { double val; int pos; int col_ind; } ITEM;
typedef ITEM * PITEM;

int compar(const void *a, const void *b)
{
  double a1 = PITEM(a)->val;
  double b1 = PITEM(b)->val;
  if(a1 < b1)
    return -1;
  if(a1 > b1)
    return 1;
  return 0;
}

void calc_vals()
{
  ITEM *arr = new ITEM[data.size()];
  for(unsigned int column_ind = 0; column_ind < MAX_ITEMS; column_ind++)
  {
    switch(vals[column_ind])
    {
    case eNoCols:
    {
      for(unsigned int row_ind = 0; row_ind < data.size(); row_ind++)
      {
        if(data[row_ind][column_ind] && strlen(data[row_ind][column_ind]))
          col_used[column_ind] = true;
      }
    }
    break;
    case eCPU:
      // gotta add support for CPU vals.  This means sorting on previous-col
      // divided by this column.  If a test run takes twice the CPU but does
      // three times the work it should be green!  If it does half the work but
      // uses the same CPU it should be red!
    break;
    case eSpeed:
    case eLatency:
    {
      for(unsigned int row_ind = 0; row_ind < data.size(); row_ind++)
      {
        arr[row_ind].val = 0.0;
        if(data[row_ind].size() <= column_ind
         || sscanf(data[row_ind][column_ind], "%lf", &arr[row_ind].val) == 0)
          arr[row_ind].val = 0.0;
        if(vals[column_ind] == eLatency && arr[row_ind].val != 0.0)
        {
          if(strstr(data[row_ind][column_ind], "ms"))
            arr[row_ind].val *= 1000.0;
          else if(!strstr(data[row_ind][column_ind], "us"))
            arr[row_ind].val *= 1000000.0; // is !us && !ms then secs!
        }
        arr[row_ind].pos = row_ind;
      }
      qsort(arr, data.size(), sizeof(ITEM), compar);
      int col_count = -1;
      double min_col = -1.0, max_col = -1.0;
      for(unsigned int sort_ind = 0; sort_ind < data.size(); sort_ind++)
      {
        // if item is different from previous or if the first row
        // (sort_ind == 0) then increment col count
        if(sort_ind == 0 || arr[sort_ind].val != arr[sort_ind - 1].val)
        {
          if(arr[sort_ind].val != 0.0)
          {
            col_count++;
            if(min_col == -1.0)
              min_col = arr[sort_ind].val;
            else
              min_col = min(arr[sort_ind].val, min_col);
            max_col = max(max_col, arr[sort_ind].val);
          }
        }
        arr[sort_ind].col_ind = col_count;
      }
      // if more than 1 line has data then calculate colors
      if(col_count > 0)
      {
        double divisor = max_col / min_col;
        if(divisor < 2.0)
        {
          double mult = sqrt(2.0 / divisor);
          max_col *= mult;
          min_col /= mult;
        }
        double range_col = max_col - min_col;
        for(unsigned int sort_ind = 0; sort_ind < data.size(); sort_ind++)
        {
          if(arr[sort_ind].col_ind > -1)
          {
            bool reverse = false;
            PCCHAR extra = "";
            if(vals[column_ind] != eSpeed)
            {
              reverse = true;
              extra = " COLSPAN=2";
            }
            props[arr[sort_ind].pos][column_ind]
                  = get_col(range_col, arr[sort_ind].val - min_col, reverse, extra);
          }
          else if(vals[column_ind] != eSpeed)
          {
            props[arr[sort_ind].pos][column_ind] = "COLSPAN=2";
          }
        }
      }
      else
      {
        for(unsigned int sort_ind = 0; sort_ind < data.size(); sort_ind++)
        {
          if(vals[column_ind] == eLatency)
          {
            props[sort_ind][column_ind] = "COLSPAN=2";
          }
        }
      }
    }
    break;
    } // end switch
  }
}

PCCHAR get_col(double range_col, double val, bool reverse, CPCCHAR extra)
{
  if(reverse)
    val = range_col - val;
  const int buf_len = 256;
  PCHAR buf = new char[buf_len];
  int green = int(255.0 * val / range_col);
  green = min(green, 255);
  int red = 255 - green;
  _snprintf(buf
#ifndef NO_SNPRINTF
          , buf_len
#endif
          , "bgcolor=\"#%02X%02X00\"%s", red, green, extra);
  buf[buf_len - 1] = '\0';
  return buf;
}

void heading(const char * const head);

int header()
{
  int vers_width = 2;
  if(col_used[COL_DATA_CHUNK_SIZE] == true)
    vers_width++;
  if(col_used[COL_CONCURRENCY] == true)
    vers_width++;
  int mid_width = 1;
  if(col_used[COL_MAX_SIZE])
    mid_width++;
  if(col_used[COL_MIN_SIZE])
    mid_width++;
  if(col_used[COL_NUM_DIRS])
    mid_width++;
  if(col_used[COL_FILE_CHUNK_SIZE])
    mid_width++;
  printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
"<HTML>"
"<HEAD><TITLE>Bonnie++ Benchmark results</TITLE>"
"<STYLE type=\"text/css\">"
"TD.header {text-align: center; backgroundcolor: \"#CCFFFF\" }"
"TD.rowheader {text-align: center; backgroundcolor: \"#CCCFFF\" }"
"TD.size {text-align: center; backgroundcolor: \"#CCCFFF\" }"
"TD.ksec {text-align: center; fontstyle: italic }"
"</STYLE>"
"<BODY>"
"<TABLE ALIGN=center BORDER=3 CELLPADDING=2 CELLSPACING=1>"
"<TR><TD COLSPAN=%d class=\"header\"><FONT SIZE=+1><B>"
"Version " BON_VERSION
"</B></FONT></TD>"
"<TD COLSPAN=6 class=\"header\"><FONT SIZE=+2><B>Sequential Output</B></FONT></TD>"
"<TD COLSPAN=4 class=\"header\"><FONT SIZE=+2><B>Sequential Input</B></FONT></TD>"
"<TD COLSPAN=2 ROWSPAN=2 class=\"header\"><FONT SIZE=+2><B>Random<BR>Seeks</B></FONT></TD>"
"<TD COLSPAN=%d class=\"header\"></TD>"
"<TD COLSPAN=6 class=\"header\"><FONT SIZE=+2><B>Sequential Create</B></FONT></TD>"
"<TD COLSPAN=6 class=\"header\"><FONT SIZE=+2><B>Random Create</B></FONT></TD>"
"</TR>\n"
"<TR>", vers_width, mid_width);
  if(col_used[COL_CONCURRENCY] == true)
    printf("<TD COLSPAN=2>Concurrency</TD>");
  else
    printf("<TD></TD>");
  printf("<TD>Size</TD>");
  if(col_used[COL_DATA_CHUNK_SIZE] == true)
    printf("<TD>Chunk Size</TD>");
  heading("Per Char"); heading("Block"); heading("Rewrite");
  heading("Per Char"); heading("Block");
  printf("<TD>Num Files</TD>");
  if(col_used[COL_MAX_SIZE])
    printf("<TD>Max Size</TD>");
  if(col_used[COL_MIN_SIZE])
    printf("<TD>Min Size</TD>");
  if(col_used[COL_NUM_DIRS])
    printf("<TD>Num Dirs</TD>");
  if(col_used[COL_FILE_CHUNK_SIZE])
    printf("<TD>Chunk Size</TD>");
  heading("Create"); heading("Read"); heading("Delete");
  heading("Create"); heading("Read"); heading("Delete");
  printf("</TR>");

  printf("<TR><TD COLSPAN=%d></TD>", vers_width);

  int i;
  CPCCHAR ksec_form = "<TD class=\"ksec\"><FONT SIZE=-2>%s/sec</FONT></TD>"
                      "<TD class=\"ksec\"><FONT SIZE=-2>%% CPU</FONT></TD>";
  for(i = 0; i < 5; i++)
  {
    printf(ksec_form, "K");
  }
  printf(ksec_form, "");
  printf("<TD COLSPAN=%d></TD>", mid_width);
  for(i = 0; i < 6; i++)
  {
    printf(ksec_form, "");
  }
  printf("</TR>\n");
  return mid_width;
}

void heading(const char * const head)
{
  printf("<TD COLSPAN=2>%s</TD>", head);
}

void footer()
{
  printf("</TABLE>\n</BODY></HTML>\n");
}

STR_VEC split(CPCCHAR delim, CPCCHAR buf)
{
  STR_VEC arr;
  char *tmp = strdup(buf);
  while(1)
  {
    arr.push_back(tmp);
    tmp = strstr(tmp, delim);
    if(!tmp)
      break;
    *tmp = '\0';
    tmp += strlen(delim);
  }
  return arr;
}

void read_in(CPCCHAR buf)
{
  STR_VEC arr = split(",", buf);
  if(strcmp(arr[0], CSV_VERSION) )
  {
    fprintf(stderr, "Can't process: %s\n", buf);
    free((void *)arr[0]);
    return;
  }
  data.push_back(arr);
}

void print_item(int num, int item)
{
  PCCHAR line_data;
  if(int(data[num].size()) > item)
    line_data = data[num][item];
  else
    line_data = "";
  if(props[num][item])
    printf("<TD %s>%s</TD>", props[num][item], line_data);
  else
    printf("<TD>%s</TD>", line_data);
}

void print_a_line(int num, int start, int end)
{
  int i;
  for(i = start; i <= end; i++)
  {
    print_item(num, i);
  }
}
