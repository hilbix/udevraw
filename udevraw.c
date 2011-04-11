/* $Header$
 *
 * This Works is placed under the terms of the Copyright Less License,
 * see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.
 *
 * (Note that tinolib in the tino/ subdirectory is not CLL!)
 *
 * $Log$
 * Revision 1.1  2011-04-11 22:36:07  tino
 * *** empty log message ***
 *
 */

#include "tino/put.h"
#include "tino/getopt.h"
#include "tino/ex.h"

#include <libudev.h>

#include "udevraw_version.h"

static struct udev *udev;
static struct udev_monitor *mon;
static struct udev_device *dev;

static int quiet;

static void
cleanup(void)
{
  if (dev)
    {
      udev_device_unref(dev);
      dev = 0;
    }
  if (mon)
    {
      udev_monitor_unref(mon);
      mon = 0;
    }
  if (udev)
    {
      udev_unref(udev);
      udev = 0;
    }
}

static void
voops(TINO_VA_LIST list)
{
  int		e;

  e = errno;
  cleanup();
  if (!quiet)
    tino_verror("udevraw OOPS", list, e);
  TINO_ABORT(23);
}

static void
oops(const char *s, ...)
{
  tino_va_list  list;

  tino_va_start(list, s);
  voops(&list);
}

static int
ok(int ret, const char *s, ...)
{
  tino_va_list  list;

  if (ret>=0)
    return ret;

  tino_va_start(list, s);
  voops(&list);
  return ret;
}

static void *
nonnull(void *ptr, const char *s, ...)
{
  tino_va_list  list;

  if (ptr)
    return ptr;

  tino_va_start(list, s);
  voops(&list);
  return NULL;
}

int
main(int argc, char **argv)
{
  int	argn;
  const char *src, *subsystem, *devtype;
  int	out;

  argn	= tino_getopt(argc, argv, 0, 0,
		      TINO_GETOPT_VERSION(UDEVRAW_VERSION)
		      "\n"
		      "	Dumps UDEV events to stdout.\n"
		      "	Unbuffered and directly usable by shells as it ought to be.\n"
		      "	Example:\n"
		      "	# while read -ru3 seq action subsys devtype device args; do\n"
		      "	#   eval $args\n"
		      "	#   ...\n"
		      "	# done 3< <(udevraw)"
		      ,

		      TINO_GETOPT_USAGE
		      "h	this help"
		      ,

		      TINO_GETOPT_STRING
		      "d dev	filter for Devtype (like 'disk')"
		      , &devtype,
		      
		      TINO_GETOPT_STRING
		      "f sys	Filter for subsystem (like 'block')"
		      , &subsystem,

		      TINO_GETOPT_STRING
		      TINO_GETOPT_DEFAULT
		      "s src	udev monitor Source (like 'kernel')"
		      , &src,
		      "udev",

		      TINO_GETOPT_INT
		      TINO_GETOPT_DEFAULT
		      "u fd	output goes to Unix fd"
		      , &out,
		      1,

		      TINO_GETOPT_FLAG
		      "q	be Quiet on errors"
		      , &quiet,

		      NULL
		      );
  if (argn<=0)
    return 1;
  
  udev = nonnull(udev_new(), "cannot open udev");
  mon = nonnull(udev_monitor_new_from_netlink(udev, src), "cannot open monitor with src=%s", src);

  if (subsystem || devtype)
    ok(udev_monitor_filter_add_match_subsystem_devtype(mon, subsystem, devtype), "cannot filter for %s/%s", subsystem, devtype);

  ok(udev_monitor_enable_receiving(mon), "cannot start receiving on monitor");
#if 0
  /* If we want to listen to kernel and udev at once,
   * we need to use fd's and use select().
   * For now only one source is possible.
   */
  int monfd = ok(udev_monitor_get_fd(mon), "bug getting monitor fd");
#endif

  tino_io_fd(out, "stdout");
  while ((dev=nonnull(udev_monitor_receive_device(mon), "cannot fetch event data from monitor"))!=0)
    {
      udev_device_unref(dev);
      tino_put_dec_l(out, 0, udev_device_get_seqnum(dev));
      tino_io_put(out, ' ');
      tino_put_ansi_if(out, udev_device_get_action(dev));
      tino_io_put(out, ' ');
      tino_put_ansi_if(out, udev_device_get_subsystem(dev));
      tino_io_put(out, ' ');
      tino_put_ansi_if(out, udev_device_get_devtype(dev));
      tino_io_put(out, ' ');
      tino_put_ansi_if(out, udev_device_get_devnode(dev));
      tino_io_put(out, '\n');
      tino_io_flush_write(out);
      dev = 0;
    }
  cleanup();
  return 0;
}

