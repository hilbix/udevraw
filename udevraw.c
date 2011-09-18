/* $Header$
 *
 * This Works is placed under the terms of the Copyright Less License,
 * see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.
 *
 * (Note that tinolib in the tino/ subdirectory is not CLL!)
 *
 * $Log$
 * Revision 1.2  2011-09-18 13:13:39  tino
 * Options -a -e -i -p
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
static struct udev_queue *queue;
static const char *prefix, *extended;
static int out;
static int quiet;

static int first;

static void
cleanup(void)
{
  if (dev)
    {
      udev_device_unref(dev);
      dev = 0;
    }
  if (queue)
    {
      udev_queue_unref(queue);
      queue = 0;
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

static void
param(const char *what, const char *mid)
{
  if (!first)
    tino_io_put(out, ' ');
  first = 0;
  if (prefix)
    tino_put_s(out, prefix);
  if (mid)
    tino_put_s(out, mid);
  tino_put_s(out, what);
  tino_io_put(out, '=');
}

static const char *
check(int force, const char *what)
{
  if (force<1)
    {
      if (!extended || (*extended!='*' && !strstr(extended, what)))
        return NULL;
    }
  if (extended && *extended)
    {
      const char *tmp;

      if (*extended=='!' && extended[1]=='*')
        return NULL;

      /* check for disabled with !what */
      tmp = strstr(extended+1, what);
      if (tmp && *--tmp=='!')
	return NULL;
    }
  if (force>=0)
    param(what, NULL);
  return what;
}

static void
outs(int force, const char *what, const char *fn(struct udev_device *))
{
  if (check(force, what))
    tino_put_ansi_if(out, fn(dev));
}

static void
lister(struct udev_list_entry *list)
{
  struct udev_list_entry *next;

  next = udev_list_entry_get_next(list);
  if (!next)
    {
      tino_put_ansi_if(out, udev_list_entry_get_name(list));
      return;
    }

  tino_put_ansi_start(out);
  tino_put_ansi(out, udev_list_entry_get_name(list), NULL);
  do
    {
      tino_put_ansi_c(out, ' ', NULL);
      tino_put_ansi(out, udev_list_entry_get_name(next), NULL);
      next = udev_list_entry_get_next(next);
    } while (next);
  tino_put_ansi_end(out);
}

static void
lister2(struct udev_device *dev, const char *tag,
	struct udev_list_entry *fn(struct udev_device *),
	const char *val(struct udev_device *, const char *))
{
  struct udev_list_entry *list;

  for (list=fn(dev); list; list=udev_list_entry_get_next(list))
    {
      const char *name;

      name = udev_list_entry_get_name(list);
      param(name, tag);
      tino_put_ansi_if(out, val(dev,name));
    }
}

int
main(int argc, char **argv)
{
  int	argn;
  const char *src, *subsystem, *devtype, *action;
  int	idle;

  argn	= tino_getopt(argc, argv, 0, 0,
		      TINO_GETOPT_VERSION(UDEVRAW_VERSION)
		      "\n"
		      "	Dumps UDEV events to stdout.\n"
		      "	Unbuffered and directly usable by shells as it ought to be.\n"
		      "	Example:\n"
		      "	# while read -ru3 args; do\n"
		      "	#   eval \"$args\"\n"
		      "	#   ...\n"
		      "	# done 3< <(udevraw)"
		      ,

		      TINO_GETOPT_USAGE
		      "h	this help"
		      ,

		      TINO_GETOPT_STRING
		      "a actn	filter for Action (like 'change')"
		      , &action,

		      TINO_GETOPT_STRING
		      "d dev	filter for Devtype (like 'disk')"
		      , &devtype,

		      TINO_GETOPT_STRING
		      "e attr	Enable extended output (like 'syspath')\n"
		      "		'!att' to suppress output, start with '*' for all"
		      , &extended,
		      
		      TINO_GETOPT_STRING
		      "f sys	Filter for subsystem (like 'block')"
		      , &subsystem,

		      TINO_GETOPT_FLAG
		      "i	only output Idle state"
		      , &idle,

		      TINO_GETOPT_STRING
		      "p prfx	set Prefix for variables"
		      , &prefix,

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

  queue = nonnull(udev_queue_new(udev), "cannot get udev queue");

  tino_io_fd(out, out==1 ? "stdout" : "fd");
  while ((dev=nonnull(udev_monitor_receive_device(mon), "cannot fetch event data from monitor"))!=0)
    {
#if 0
      tino_put_dec(out, 0, udev_queue_get_udev_is_active(queue));
      tino_io_put(out, ' ');
#endif
      if (idle && !udev_queue_get_queue_is_empty(queue))
	continue;
      if (action && strcmp(action, udev_device_get_action(dev)))
        continue;

      first = 1;

      if (check(!idle, "idle"))
        tino_put_dec(out, 0, udev_queue_get_queue_is_empty(queue));
      if (check(1, "seq"))
        tino_put_dec_l(out, 0, udev_device_get_seqnum(dev));

      outs(!action,    "action",  udev_device_get_action);
      outs(!subsystem, "subsys",  udev_device_get_subsystem);
      outs(!devtype,   "devtype", udev_device_get_devtype);
      outs(1,          "devnode", udev_device_get_devnode);
      outs(0,          "syspath", udev_device_get_syspath);
      outs(0,          "sysname", udev_device_get_sysname);
      outs(0,          "sysnum",  udev_device_get_sysnum);
      outs(0,          "driver",  udev_device_get_driver);

      if (check(0, "devlinks"))
	lister(udev_device_get_devlinks_list_entry(dev));
      if (check(0, "tags"))
	lister(udev_device_get_tags_list_entry(dev));

      if (check(-1, "properties"))
	lister2(dev, "prop_", udev_device_get_properties_list_entry, udev_device_get_property_value);
#if 0
struct udev_list_entry *udev_device_get_sysattr_list_entry(struct udev_device *);

      if (check(-1, "sysattr"))
	lister2(dev, "sys_", udev_device_get_sysattr_list_entry, udev_device_get_sysattr_value);
#endif
      tino_io_put(out, '\n');
      tino_io_flush_write(out);

      udev_device_unref(dev);
      dev = 0;
    }
  cleanup();
  return 0;
}

