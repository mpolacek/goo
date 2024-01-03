/* Lists.  Use C23.  */

#include <error.h>
#include <stddef.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

typedef struct node_t node;
typedef int value_t;

struct node_t {
  value_t value;
  node *next;
};

/* Allocate N bytes of memory dynamically, with error checking.  */

static void *
xmalloc (size_t n)
{
  void *p = malloc (n);
  if (!p)
    error (EXIT_FAILURE, 0, "memory exhausted");
  return p;
}

/* Print a list LP.  */

static void
print_list (node *lp)
{
  while (lp)
    {
      printf ("%d ", lp->value);
      lp = lp->next;
    }
  putchar ('\n');
}

/* Create a new item with value V.  */

static node *
new_item (value_t v)
{
  node *n = xmalloc (sizeof (node));
  n->value = v;
  n->next = nullptr;
  return n;
}

/* Add a new element N to the front of list LP.  */

static node *
add_front (node *lp, node *n)
{
  n->next = lp;
  return n;
}

/* Add a new element N to the end of list LP.  This is O(n).  */

static node *
add_end (node *lp, node *n)
{
  node **indirect = &lp;
  while (*indirect)
    indirect = &(*indirect)->next;
  *indirect = n;
  return lp;
}

/* Sequential search for VAL in list LP.  This is O(n).  */

static node *
lookup (node *lp, value_t val)
{
  while (lp && lp->value != val)
    lp = lp->next;
  return lp;
}

/* Print value using format in ARG.  */

static void
print_v (node *lp, void *arg)
{
  char *fmt = (char *) arg;
  printf (fmt, lp->value);
}

/* Execute FN for each element of LP.  */

static void
apply (node *lp, void (*fn)(node *, void *), void *arg)
{
  for (; lp; lp = lp->next)
    (*fn) (lp, arg);
}

/* Increment counter *ARG.  */

static void
inc_counter (node *, void *arg)
{
  int *ip = (int *) arg;
  (*ip)++;
}

/* Free all elements of LP.  */

static void
free_all (node *lp)
{
  node *next;
  for (; lp; lp = next)
    {
      next = lp->next;
      free (lp);
    }
}

/* Delete first element with VAL from LP.  */

static node *
delete_item (node *lp, value_t val)
{
  node *prev = nullptr;
  for (node *p = lp; p; p = p->next)
    {
      if (p->value == val)
	{
	  if (!prev)
	    lp = p->next;
	  else
	    prev->next = p->next;
	  free (p);
	  return lp;
	}
      prev = p;
    }
  return nullptr;
}

/* Insert node N in list LP after the first node with value VAL.  */

static node *
insert_after (node *lp, node *n, value_t val)
{
  node *v = lookup (lp, val);
  if (v)
    {
      n->next = v->next;
      v->next = n;
      return lp;
    }
  return nullptr;
}

/* Insert node N in list LP before the first node with value VAL.  */

static node *
insert_before (node *lp, node *n, value_t val)
{
  node *prev = nullptr;
  for (node *p = lp; p; p = p->next)
    {
      if (p->value == val)
	{
	  if (!prev)
	    {
	      n->next = lp;
	      lp = n;
	    }
	  else
	    {
	      n->next = prev->next;
	      prev->next = n;
	    }
	  return lp;
	}
      prev = p;
    }
  return nullptr;
}

/* Split list LP after node with value VAL; return head of the second list.  */

static node *
split (node *lp, value_t val)
{
  node *prev = nullptr;
  for (node *p = lp; p; p = p->next)
    {
      if (p->value == val)
	{
	  if (prev)
	    {
	      prev->next = nullptr;
	      return p;
	    }
	  else
	    /* Can't split; leave as-is.  */
	    return lp;
	}
      prev = p;
    }
  return nullptr;
}

/* Merge lists L1 and L2.  */

static node *
merge (node *l1, node *l2)
{
  return add_end (l1, l2);
}

/* Reverse list L.  Iterative.  */

static node *
reverse_it (node *l)
{
  node *prev = nullptr;
  for (node *p = l; p;)
    {
      node *next = p->next;
      p->next = prev;
      prev = p;
      p = next;
    }
  return prev;
}

/* Worker for reverse.  */

static node *
reverse_rec (node *l, node *prev)
{
  if (!l)
    return prev;
  node *r = reverse_rec (l->next, l);
  l->next = prev;
  return r;
}

/* Reverse list L.  Recursive.  */

static node *
reverse (node *l)
{
  return reverse_rec (l, /*prev=*/nullptr);
}

int
main (void)
{
  /* We use no threads here which can interfere with handling a stream.  */
  __fsetlocking (stdout, FSETLOCKING_BYCALLER);
  __fsetlocking (stderr, FSETLOCKING_BYCALLER);

  node *l = nullptr;

  l = add_end (l, new_item (4));
  l = add_front (l, new_item (3));
  l = add_front (l, new_item (2));
  l = add_front (l, new_item (1));
  l = add_end (l, new_item (5));
  l = add_front (l, new_item (0));
  print_list (l);
  if (lookup (l, 6))
    abort ();
  if (!lookup (l, 5))
    abort ();
  apply (l, print_v, "[%x]");
  putchar ('\n');
  int n = 0;
  apply (l, inc_counter, &n);
  if (n != 6)
    abort ();

  l = delete_item (l, 0);
  l = delete_item (l, 5);
  print_list (l);

  if (delete_item (l, 7))
    abort ();

  l = insert_after (l, new_item (11), 1);
  l = insert_after (l, new_item (33), 3);
  l = insert_after (l, new_item (44), 4);
  print_list (l);
  if (insert_after (l, new_item (4), 42))
    abort ();

  insert_after (nullptr, new_item (4), 4);

  l = insert_before (l, new_item (0), 1);
  l = insert_before (l, new_item (10), 11);
  print_list (l);
  insert_before (nullptr, new_item (4), 4);

  node *l2 = split (l, 2);
  print_list (l);
  print_list (l2);
  node *l3 = split (l2, 2);
  print_list (l2);
  print_list (l3);
  l = merge (l, l2);
  print_list (l);
  free_all (l);
  l = nullptr;

  puts ("reverse");
  l = add_front (l, new_item (1));
  print_list (l);
  l = reverse_it (l);
  print_list (l);
  l = reverse (l);
  print_list (l);
  l = add_end (l, new_item (2));
  l = add_end (l, new_item (3));
  l = add_end (l, new_item (4));
  l = add_end (l, new_item (5));
  /* NB: We can't lose the pointer to the head, so can't do
     print_list (reverse_it (l)).  */
  print_list (l);
  l = reverse_it (l);
  print_list (l);
  l = reverse_it (l);
  print_list (l);
  l = reverse (l);
  print_list (l);
  l = reverse (l);
  print_list (l);

  reverse (nullptr);
  reverse_it (nullptr);

  free_all (l);
  l = nullptr;
}
