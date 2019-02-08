

static inline int
sptime(char *b, int max)
{
  int rc;
  struct timeval tv;
  struct tm * now;
  if (b == NULL) return  -1;

  gettimeofday(&tv,NULL);
  now = localtime((time_t *) &tv.tv_sec);
  
  rc = snprintf(b, max, "%4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3ld",
		now->tm_year+1900, now->tm_mon+1, now->tm_mday,
		now->tm_hour, now->tm_min, now->tm_sec, tv.tv_usec/1000);
  return rc;
}

#define debug(...) do { char ts[80]; sptime(ts, 80), fprintf (stderr, "DBG [%s] ", ts); fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n"); } while(0);

#define info(...) do { char ts[80]; sptime(ts, 80), fprintf (stderr, "INF [%s] ", ts); fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n"); } while(0);

#define error(...) do { char ts[80]; sptime(ts, 80), fprintf (stderr, "WRN [%s] ", ts); fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n"); } while(0);

#define error(...) do { char ts[80]; sptime(ts, 80), fprintf (stderr, "ERR [%s] ", ts); fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n"); } while(0);


int callback_midway_ws(
		       struct lws *wsi,
		       enum lws_callback_reasons reason,
		       void *user,
		       void *in,
		       size_t len
		       );


int queue_message(void);

int drain_queue(void);
