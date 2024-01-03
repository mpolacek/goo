#include <iostream>
#include <vector>
#include <deque>
#include <climits>

struct node
{
  int val;
  node *left;
  node *right;
};

static node *
new_node (int i)
{
  node *n = new node;
  n->val = i;
  n->left = n->right = nullptr;
  return n;
}

static void
inorder_1 (node *root)
{
  if (root == nullptr)
    return;
  inorder_1 (root->left);
  std::cout << root->val << " ";
  inorder_1 (root->right);
}

static void
inorder (node *root)
{
  inorder_1 (root);
  std::cout << "\n";
}

static void
postorder_1 (node *root)
{
  if (root == nullptr)
    return;
  postorder_1 (root->left);
  postorder_1 (root->right);
  std::cout << root->val << " ";
}

static void
postorder (node *root)
{
  postorder_1 (root);
  std::cout << "\n";
}

static void
preorder_1 (node *root)
{
  if (root == nullptr)
    return;
  std::cout << root->val << " ";
  preorder_1 (root->left);
  preorder_1 (root->right);
}

static void
preorder (node *root)
{
  preorder_1 (root);
  std::cout << "\n";
}

static node *
buildp (int parent[], int n)
{
  std::vector<node *> v(n);
  node *root = nullptr;
  for (int i = 0; i < n; i++)
    v[i] = new_node (i);

  for (int i = 0; i < n; i++)
    {
      if (parent[i] == -1)
	root = v[i];
      else
	{
	  if (v[parent[i]]->left == nullptr)
	    v[parent[i]]->left = v[i];
	  else
	    v[parent[i]]->right = v[i];
	}
    }

  return root;
}

static int
find_root (int a[], int root, int n)
{
  for (int i = 0; i < n; i++)
    if (a[i] == root)
      return i;
  return -1;
}

static node *
buildpost (int in[], int post[], int n)
{
  node *root = new_node (post[n - 1]);
  int m = find_root (in, post[n - 1], n);
  if (m > 0)
    root->left = buildpost (in, post, m);
  if (m < n - 1)
    root->right = buildpost (in + m + 1, post + m, n - m - 1);
  return root;
}

static int
left_sum (node *root)
{
  if (!root)
    return 0;
  int lsum = left_sum (root->left);
  int rsum = left_sum (root->right);
  root->val += lsum;
  return root->val + rsum;
}

static void
mirror (node *root)
{
  if (!root)
    return;
  node *tmp = root->left;
  root->left = root->right;
  root->right = tmp;
  mirror (root->left);
  mirror (root->right);
}

static bool
id (node *a, node *b)
{
  if (!a && !b)
    return true;
  if (!a || !b)
    return false;
  return (a->val == b->val
	  && id (a->left, b->left)
	  && id (a->right, b->right));
}

static bool
has_sum_1 (node *t, int k, int sum)
{
  if (!t)
    return false;
  sum += t->val;
  if (!t->left && !t->right && sum == k)
    return true;
  return has_sum_1 (t->left, k, sum) || has_sum_1 (t->right, k, sum);
}

static bool
has_sum (node *t, int sum)
{
  return has_sum_1 (t, sum, 0);
}

static int
check_height (node *t)
{
  if (!t)
    return 0;
  int l = check_height (t->left);
  if (l == -1)
    return -1;
  int r = check_height (t->right);
  if (r == -1)
    return -1;
  if (std::abs (l - r) > 1)
    return -1;
  return std::max (l, r) + 1;
}

static bool
balanced_p (node *t)
{
  return check_height (t) != -1;
}

static int
height (node *t)
{
  if (!t)
    return 0;
  return std::max (height (t->left), height (t->right)) + 1;
}

static int
diameter (node *t)
{
  if (t == nullptr)
    return 0;
  int diam = 1 + height (t->left) + height (t->right);
  return std::max (diam, std::max (diameter (t->left), diameter (t->right)));
}

static int
diameter2 (node *t, int *height)
{
  if (!t)
    return 0;
  int lh = 0, rh = 0;
  int ldiam = diameter2 (t->left, &lh);
  int rdiam = diameter2 (t->right, &rh);
  *height = std::max (lh, rh) + 1;
  return std::max (lh + rh + 1, std::max (ldiam, rdiam));
}

static bool
sum_p (node *t)
{
  if (!t || (!t->left && !t->right))
    return true;
  int l = t->left ? t->left->val : 0;
  int r = t->right ? t->right->val : 0;
  if (t->val != (l + r))
    return false;
  return sum_p (t->left) && sum_p (t->right);
}

static bool
is_leaf (node *t)
{
  return t && !t->left && !t->right;
}

static bool
sum_tree_p (node *t)
{
  if (!t || (!t->left && !t->right))
    return true;
  int l = t->left ? t->left->val : 0;
  if (!is_leaf (t->left))
    l *= 2;
  int r = t->right ? t->right->val : 0;
  if (!is_leaf (t->right))
    r *= 2;
  if (t->val != l + r)
    return false;
  return sum_tree_p (t->left) && sum_tree_p (t->right);
}

bool
ancestors (node *t, int val)
{
  if (!t)
    return false;
  if (t->val == val)
    return true;
  if (ancestors (t->left, val) || ancestors (t->right, val))
    {
      std::cout << t->val << "\n";
      return true;
    }
  return false;
}

static int
level_r (node *t, int val, int level)
{
  if (!t)
    return 0;
  if (t->val == val)
    return level;
  int l = level_r (t->left, val, level + 1);
  if (l != 0)
    return l;
  l = level_r (t->right, val, level + 1);
  return l;
}

static void
level (node *t, int val)
{
  std::cout << "level: " << level_r (t, val, 1) << "\n";
}

static void
print_level_r (node *t, int l, int clev)
{
  if (!t)
    return;
  if (clev == l)
    {
      std::cout << t->val << " ";
      return;
    }
  print_level_r (t->left, l, clev + 1);
  print_level_r (t->right, l, clev + 1);
}

static void
print_level (node *t, int l)
{
  print_level_r (t, l, 0);
  std::cout << "\n";
}

static void
duplicate (node *t)
{
  if (!t)
    return;
  node *n = new_node (t->val);
  node *lp = t->left;
  t->left = n;
  n->left = lp;
  duplicate (n->left);
  duplicate (t->right);
}

static void
print_stk (const std::deque<node *> &d)
{
  for (auto n : d)
    std::cout << n->val << " ";
  std::cout << "\n";
}

static void
print_paths_1 (node *t, std::deque<node *> &d)
{
  if (!t)
    return;
  d.push_back (t);
  if (!t->left && !t->right)
    print_stk (d);
  else
    {
      print_paths_1 (t->left, d);
      print_paths_1 (t->right, d);
    }
  d.pop_back ();
}

static void
print_paths (node *t)
{
  std::deque<node *> d;
  print_paths_1 (t, d);
}

static void
print_max_path (node *n, node *leaf, std::deque<node *> &d)
{
  if (!n)
    return;
  d.push_back (n);
  if (n == leaf)
    print_stk (d);
  else
    {
      print_max_path (n->left, leaf, d);
      print_max_path (n->right, leaf, d);
    }
  d.pop_back ();
}

static void
find_max_leaf (node *n, std::deque<node *> &d, int &max, node **leaf)
{
  if (!n)
    return;
  d.push_back (n);
  if (!n->left && !n->right)
    {
      int cmax = 0;
      for (auto x : d)
	cmax += x->val;
      if (cmax > max)
	{
	  max = cmax;
	  *leaf = n;
	}
    }
  else
    {
      find_max_leaf (n->left, d, max, leaf);
      find_max_leaf (n->right, d, max, leaf);
    }
  d.pop_back ();
}

static void
max_path (node *n)
{
  if (!n)
    return;
  std::deque<node *> d;
  int max = -999999;
  node *leaf = nullptr;
  find_max_leaf (n, d, max, &leaf);
  d.clear ();
  print_max_path (n, leaf, d);
}

static void
get_sum_paths_1 (node *n, int &sum, std::deque<node *> &d)
{
  if (!n)
    return;
  d.push_back (n);
  if (!n->left && !n->right)
    {
      int s = 0;
      int t = 1;
      for (auto it = d.rbegin (); it != d.rend (); it++)
	{
	  s += t * (*it)->val;
	  t *= 10;
	}
      sum += s;
    }
  else
    {
      get_sum_paths_1 (n->left, sum, d);
      get_sum_paths_1 (n->right, sum, d);
    }
  d.pop_back ();
}

static int
get_sum_paths (node *n)
{
  int sum = 0;
  std::deque<node *> d;
  get_sum_paths_1 (n, sum, d);
  return sum;
}

static node *
get_right (node *t, int val)
{
  if (!t)
    return nullptr;
  std::deque<node *> d;
  d.push_front (t);
  bool get_next = false;
  while (!d.empty ())
    {
      int sz = d.size ();
      while (sz-- > 0)
	{
	  node *n = d.back ();
	  d.pop_back ();
	  get_next |= n->val == val;
	  if (get_next)
	    return sz == 0 ? nullptr : d.front ();
	  if (n->left)
	    d.push_front (n->left);
	  if (n->right)
	    d.push_front (n->right);
	}
    }
  return nullptr;
}

static void
find_deep_left (node *root, node **n, int level, int &maxlevel, bool is_left)
{
  if (!root)
    return;
  if (is_left && !root->left && !root->right)
    {
      if (level > maxlevel)
	{
	  *n = root;
	  maxlevel = level;
	}
    }
  find_deep_left (root->left, n, level + 1, maxlevel, true);
  find_deep_left (root->right, n, level + 1, maxlevel, false);
}

static node *
deep_left (node *t)
{
  node *n = nullptr;
  int maxlevel = 0;
  find_deep_left (t, &n, 0, maxlevel, true);
  return n;
}

static void
left_view_1 (node *t, int &level_done, int clev)
{
  if (!t)
    return;
  if (level_done < clev)
    {
      std::cout << t->val << "\n";
      level_done = clev;
    }
  left_view_1 (t->left, level_done, clev + 1);
  left_view_1 (t->right, level_done, clev + 1);
}

static void
left_view (node *t)
{
  int level_done = 0;
  left_view_1 (t, level_done, 1);
}

static bool
same_level_1 (node *t, int &leaf_level, int clev)
{
  if (!t)
    return true;
  if (!t->left && !t->right)
    {
      if (leaf_level == 0)
	leaf_level = clev;
      else if (leaf_level != clev)
	return false;
    }
  return (same_level_1 (t->left, leaf_level, clev + 1)
	  && same_level_1 (t->right, leaf_level, clev + 1));
}

static bool
same_level (node *t)
{
  int leaf_level = 0;
  return same_level_1 (t, leaf_level, 1);
}

static int
find_max (node *t)
{
  if (!t)
    return INT_MIN;
  int m = std::max (find_max (t->left), find_max (t->right));
  return std::max (m, t->val);
}

static bool
find_path (node *t, int a, std::vector<int> &v)
{
  if (!t)
    return false;
  v.push_back (t->val);
  if (t->val == a)
    return true;
  if (find_path (t->left, a, v) || find_path (t->right, a, v))
    return true;
  v.pop_back ();
  return false;
}

static int
dist (node *t, int a, int b)
{
  std::vector<int> pa, pb;
  if (!find_path (t, a, pa) || !find_path (t, b, pb))
    return -1;
  int i = 0;
  for (; i < (int) pa.size () && i < (int) pb.size (); i++)
    if (pa[i] != pb[i])
      break;
  return pa.size () - i + pb.size () - i;
}

static int
fill_height (int parent[], int h[], int i)
{
  if (parent[i] == -1)
    h[i] = 1;
  else if (h[i] == 0)
    h[i] = fill_height (parent, h, parent[i]) + 1;
  return h[i];
}

static int
get_height (int parent[], int n)
{
  int *h = new int[n];
  for (int i = 0; i < n; ++i)
    h[i] = 0;
  for (int i = 0; i < n; ++i)
    fill_height (parent, h, i);
  int r = h[0];
  for (int i = 1; i < n; i++)
    if (h[i] > r)
      r = h[i];
  return r;
}

static void
print_levels (node *t, int lo, int hi)
{
  if (!t)
    return;
  std::deque<node *> d;
  d.push_front (t);
  int clev = 1;
  while (!d.empty ())
    {
      int sz = d.size ();
      while (sz-- > 0)
	{
	  node *n = d.back ();
	  d.pop_back ();
	  if (clev >= lo && clev <= hi)
	    std::cout << n->val << " ";
	  if (n->left)
	    d.push_front (n->left);
	  if (n->right)
	    d.push_front (n->right);
	}
      clev++;
      std::cout << "\n";
    }
}

static void
print_range (const std::vector<int> &buffer, int l, int h)
{
  for (int i = l; i <= h; i++)
    std::cout << buffer[i] << " ";
  std::cout << "\n";
}

static void
find_sum (node *t, int sum, std::vector<int> &buffer)
{
  if (!t)
    return;
  buffer.push_back (t->val);
  int tmp = sum;
  for (int i = buffer.size () - 1; i >= 0; i--)
    {
      tmp -= buffer[i];
      if (tmp == 0)
	print_range (buffer, i, buffer.size () - 1);
    }
  find_sum (t->left, sum, buffer);
  find_sum (t->right, sum, buffer);
  buffer.pop_back ();
}

int
main ()
{
  int parent[] = { 1, 5, 5, 2, 2, -1, 3 };
  node *root1 = buildp (parent, 7);
  preorder (root1);
  inorder (root1);
  postorder (root1);

  std::cout << "build from in + post\n";
  int in[] = { 4, 8, 2, 5, 1, 6, 3, 7 };
  int post[] = { 8, 4, 5, 2, 6, 7, 3, 1 };
  node *root2 = buildpost (in, post, 8);
  preorder (root2);
  inorder (root2);
  postorder (root2);

  std::cout << "left sum before:\n";
  node *root = new_node(1);
  root->left = new_node(2);
  root->right = new_node(3);
  root->left->left = new_node(4);
  root->left->right = new_node(5);
  root->right->right = new_node(6);
  preorder (root);
  inorder (root);
  postorder (root);
  std::cout << "left sum after:\n";
  left_sum (root);
  preorder (root);
  inorder (root);
  postorder (root);
  std::cout << "mirror orig \n";
  node *root3 = new_node (1);
  root3->left = new_node (3);
  root3->right = new_node (2);
  root3->right->left = new_node (5);
  root3->right->right = new_node (4);
  preorder (root3);
  inorder (root3);
  postorder (root3);
  std::cout << "mirrored\n";
  mirror (root3);
  preorder (root3);
  inorder (root3);
  postorder (root3);

  if (!id (root3, root3))
    __builtin_abort ();
  if (id (root3, root))
    __builtin_abort ();

  node *root4 = new_node (10);
  root4->left = new_node (8);
  root4->right = new_node (2);
  root4->left->left = new_node (3);
  root4->left->right = new_node (5);
  root4->right->right = new_node (2);
  if (!has_sum (root4, 21))
    __builtin_abort ();
  if (!has_sum (root4, 23))
    __builtin_abort ();
  if (!has_sum (root4, 14))
    __builtin_abort ();
  if (has_sum (root4, 18))
    __builtin_abort ();

  node *root5 = new_node(1);
  root5->left = new_node(2);
  root5->right = new_node(3);
  root5->left->left = new_node(4);
  root5->left->right = new_node(5);
  root5->left->left->left = new_node(8);
  if (balanced_p (root5))
    __builtin_abort ();

  node *root6 = new_node(1);
  root6->left        = new_node(2);
  root6->right       = new_node(3);
  root6->left->left  = new_node(4);
  root6->left->right = new_node(5);
  std::cout << "diameter: " << diameter (root6) << "\n";
  int h = 0;
  std::cout << "diameter: " << diameter2 (root6, &h) << "\n";

  node *root7  = new_node(10);
  root7->left         = new_node(8);
  root7->right        = new_node(2);
  root7->left->left   = new_node(3);
  root7->left->right  = new_node(5);
  root7->right->right = new_node(2);
  if (sum_p (root7))
    std::cout << "sum ok\n";
  else
    std::cout << "sum NOT ok\n";

  node *root8 = new_node (40);
  root8->left = new_node (12);
  root8->right = new_node (8);
  root8->left->left = new_node (2);
  root8->left->right = new_node (4);
  root8->left->left->left = new_node (1);
  root8->left->left->right = new_node (1);
  root8->left->right->left = new_node (2);
  root8->left->right->right = new_node (2);
  root8->right->left = new_node (3);
  root8->right->right = new_node (5);
  if (sum_tree_p (root8))
    std::cout << "sum tree ok\n";
  else
    std::cout << "sum tree NOT ok\n";

  node *root9 = new_node (1);
  root9->right = new_node (3);
  root9->left = new_node (2);
  root9->left->right = new_node (5);
  root9->left->left = new_node (4);
  root9->left->left->left = new_node (7);
  ancestors (root9, 7);
  level (root9, 7);
  level (root9, 17);
  level (root9, 4);
  level (root9, 5);
  level (root9, 1);
  level (root9, 2);
  level (root9, 3);

  std::cout << "print_level:\n";
  print_level (root9, 0);
  print_level (root9, 1);
  print_level (root9, 2);
  print_level (root9, 3);
  print_level (root9, 4);

  std::cout << "duplicate:\n";
  node *root10 = new_node (1);
  root10->left = new_node (2);
  root10->right = new_node (3);
  root10->left->left = new_node (4);
  root10->left->right = new_node (5);
  duplicate (root10);
  preorder (root10);
  inorder (root10);
  postorder (root10);

  std::cout << "print_paths:\n";
  node *root11 = new_node (10);
  root11->left = new_node (8);
  root11->right = new_node (2);
  root11->right->left = new_node (2);
  root11->left->left = new_node (3);
  root11->left->right = new_node (5);
  print_paths (root11);

  std::cout << "max_path:\n";
  node *root12 = new_node (10);
  root12->left = new_node (2);
  root12->right = new_node (7);
  root12->left->left = new_node (8);
  root12->left->right = new_node (-4);
  max_path (root12);

  std::cout << "max sum paths:\n";
  node *root13 = new_node (6);
  root13->left = new_node (3);
  root13->right = new_node (5);
  root13->right->right = new_node (4);
  root13->left->left = new_node (2);
  root13->left->right = new_node (5);
  root13->left->right->left = new_node (7);
  root13->left->right->right = new_node (4);
  std::cout << get_sum_paths (root13) << "\n";

  std::cout << "get right\n";
  node *root14 = new_node (10);
  root14->left = new_node (2);
  root14->right = new_node (6);
  root14->right->right = new_node (5);
  root14->left->left = new_node (8);
  root14->left->right = new_node (4);
  std::cout << "2: " << get_right (root14, 2)->val << "\n";
  std::cout << "4: " << get_right (root14, 4)->val << "\n";
  std::cout << "6: " << (get_right (root14, 6) == nullptr ? "null" : "FAIL") << "\n";

  std::cout << "deep left\n";
  node *root15 = new_node (1);
  root15->left = new_node (2);
  root15->left->left = new_node (4);
  root15->right = new_node (3);
  root15->right->left = new_node (5);
  root15->right->right = new_node (6);
  root15->right->right->right = new_node (8);
  root15->right->right->right->right = new_node (10);
  root15->right->left->right = new_node (7);
  root15->right->left->right->left = new_node (9);
  std::cout << deep_left (root15)->val << "\n";
  std::cout << "left view\n";
  left_view (root15);
  std::cout << "same level\n";
  std::cout << same_level (root15) << "\n";
  node *root16 = new_node (1);
  root16->left = new_node (2);
  root16->right = new_node (3);
  std::cout << same_level (root16) << "\n";
  std::cout << find_max (root16) << "\n";
  std::cout << find_max (root15) << "\n";

  std::cout << "dist\n";
  node *root17 = new_node (1);
  root17->left = new_node (2);
  root17->right = new_node (3);
  root17->left->left = new_node (4);
  root17->left->right = new_node (5);
  root17->right->left = new_node (6);
  root17->right->right = new_node (7);
  root17->right->left->right = new_node (8);
  std::cout << dist (root17, 4, 5) << "\n";
  std::cout << dist (root17, 4, 6) << "\n";
  std::cout << dist (root17, 2, 4) << "\n";

  std::cout << "height parent\n";
  int parent2[7] = { 1, 5, 5, 2, 2, -1, 3 };
  std::cout << get_height (parent2, 7) << "\n";

  std::cout << "print levels\n";
  node *root18 = new_node (20);
  root18->left = new_node (8);
  root18->right = new_node (22);
  root18->left->left = new_node (4);
  root18->left->right = new_node (12);
  root18->left->right->left = new_node (10);
  root18->left->right->right = new_node (14);
  print_levels (root18, 2, 4);

  node *root19 = new_node (8);
  root19->left = new_node (3);
  root19->right = new_node (2);
  root19->left->left = new_node (6);
  root19->left->right = new_node (1);
  root19->right->left = new_node (7);
  std::vector<int> v;
  std::cout << "find sum\n";
  find_sum (root19, 9, v);
}
