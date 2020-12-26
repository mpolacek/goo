// Singly linked list.

#include <utility>
#include <cstddef>

struct node {
  int val;
  node *next;
};

/* Print the values of the list L.  */

static void
print_list (node *l)
{
  while (l)
    {
      __builtin_printf ("%d%s", l->val, l->next ? " -> " : "");
      l = l->next;
    }
  __builtin_printf ("\n");
}

/* Prepend a node with value VAL to the list HEAD.  */

static void
prepend (node **head, int val)
{
  *head = new node{val, *head};
}

/* Append a node with value VAL to the list HEAD.  */

static void
append (node **head, int val)
{
  node *l = new node{val, nullptr};
  node **indirect = head;
  while (*indirect)
    indirect = &(*indirect)->next;
  *indirect = l;
}

/* Reverse the list HEAD; modifies the links.  */

static void
reverse (node **head)
{
  node *prev = nullptr, *cur = *head;

  while (cur)
    {
      node *next = cur->next;
      cur->next = prev;
      prev = cur;
      cur = next;
    }
  *head = prev;
}

/* Get the length of the LIST.  */

static int
list_length (node *list)
{
  int n = 0;
  while (list)
    ++n, list = list->next;
  return n;
}

/* Reverse the list LIST in place, i.e., without modifying the links.  */

static void
reverse_in_place (node *list)
{
  const int len = list_length (list);
  if (len < 2)
    return;
  int pos = len - 1;
  while (pos > 0)
    {
      // Swap list[cur] and list[cur + pos] values.
      node *n = list;
      // Advance N by POS.
      for (size_t i = pos; i > 0; --i)
	n = n->next;
      std::swap (list->val, n->val);
      pos -= 2;
      list = list->next;
    }
}

/* Free all the nodes of the list HEAD.  */

static void
dispose (node **head)
{
  while (*head)
    {
      node *cur = *head;
      *head = (*head)->next;
      delete cur;
    }
}

int
main ()
{
  node *l = nullptr;

  prepend (&l, 4);
  prepend (&l, 3);
  prepend (&l, 2);
  prepend (&l, 1);
  append (&l, 5);
  __builtin_printf ("initial:\n");
  print_list (l);

  __builtin_printf ("reversed:\n");
  reverse (&l);
  print_list (l);

  __builtin_printf ("reversed in place:\n");
  reverse_in_place (l);
  print_list (l);

  dispose (&l);
}
