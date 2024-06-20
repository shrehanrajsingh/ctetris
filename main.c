/*
 * File: main.c
 * Author: Shrehan Raj Singh
 * Created: 19-06-2024
 * Description: This is a single source file for a console based Tetris game.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <windows.h>

int ROW = 20;
int COLUMN = 20;

int keyLeft = VK_LEFT;
int keyRight = VK_RIGHT;
int keyDown = VK_DOWN;
int keyShift = VK_UP;
int showScore = 1;

enum ShapeType
{
  /*
    o o o
   */
  SHAPE_VLINE,
  /*
    o
    o
    o
  */
  SHAPE_HLINE,
  /*
    o
    o o o
  */
  SHAPE_L_WITHLONGHLINE,
  /*
    o
    o
    o o
  */
  SHAPE_L_WITHLONGVLINE,
  /*
   o o o
     o
     o
  */
  SHAPE_T_VERTICAL,
  /*
   o
   o o o
   o
  */
  SHAPE_T_HORIZONTAL,
  /*
     o
     o
   o o o
  */
  SHAPE_T_MIRROR,
  /*
       o
   o o o
       o
  */
  SHAPE_T_HORIZONTAL_MIRROR,
  TOTAL_SHAPES,
};

typedef struct
{
  int type;
  int pos_r; // row
  int pos_c; // column

  int is_falling;

} shape;

// Empty space -> 0, Block -> 1
int **SCREEN;

shape SHAPES[64];
size_t shape_count = 0;

shape *curr_falling_shape = NULL;

void print_screen (void);
void clrscr (void);

void add_shape (shape);
int bounded_shapeidx (int, int);
int shape_boundary_check (shape *, int);
void drop_shape (void);

DWORD WINAPI keycatch (LPVOID);

int GAMEOVER = 0;
size_t SCORE = 0;

int
rand_range (int l1, int l2)
{
  return rand () % (l1 + 1 - l2) + l1;
}

int
main (int argc, char const *argv[])
{
  if (argc == 1)
    {
    L1:;
      SCREEN = malloc (ROW * sizeof (int *));

      for (size_t i = 0; i < ROW; i++)
        {
          SCREEN[i] = malloc (COLUMN * sizeof (int));

          for (size_t j = 0; j < COLUMN; j++)
            SCREEN[i][j] = 0;
        }

      srand (time (NULL));

      DWORD keyThID;
      HANDLE keyHandle = CreateThread (NULL, 0, keycatch, NULL, 0, &keyThID);

      assert (keyHandle != NULL);

      // add a test shape
      add_shape ((shape){
          .pos_c = 1,
          .pos_r = 1,
          .type = SHAPE_HLINE,
          .is_falling = 1,
      });
      curr_falling_shape = SHAPES;

      while (!GAMEOVER)
        {
          clrscr ();
          print_screen ();

          drop_shape ();

          Sleep (200);
        }

      clrscr ();
      print_screen ();

      CloseHandle (keyHandle);
    }
  else
    {
      int helpFlag = 0;

      for (size_t i = 0; i < argc; i++)
        {
          const char *s = argv[i];

          if (!strcmp (s, "--help") || !strcmp (s, "-h"))
            helpFlag = 1;

          else if (!strcmp (s, "--width"))
            {
              assert (i < argc - 1);
              ROW = atoi (argv[i + 1]);
            }

          else if (!strcmp (s, "--height"))
            {
              assert (i < argc - 1);
              COLUMN = atoi (argv[i + 1]);
            }

          else if (!strcmp (s, "--key-left") || !strcmp (s, "-kl"))
            {
              assert (i < argc - 1);
              keyLeft = (int)*argv[i + 1];
            }

          else if (!strcmp (s, "--key-right") || !strcmp (s, "-kr"))
            {
              assert (i < argc - 1);
              keyRight = (int)*argv[i + 1];
            }

          else if (!strcmp (s, "--key-down") || !strcmp (s, "-kd"))
            {
              assert (i < argc - 1);
              keyDown = (int)*argv[i + 1];
            }

          else if (!strcmp (s, "--key-shift") || !strcmp (s, "-ks"))
            {
              assert (i < argc - 1);
              keyShift = (int)*argv[i + 1];
            }

          else if (!strcmp (s, "--show-score") || !strcmp (s, "-ss"))
            {
              assert (i < argc - 1);
              const char *p = argv[i + 1];

              if (!strcmp (p, "no") || !strcmp (p, "NO") || !strcmp (p, "No")
                  || !strcmp (p, "0") || !strcmp (p, "false")
                  || !strcmp (p, "FALSE") || !strcmp (p, "False"))
                showScore = 0;

              else if (!strcmp (p, "yes") || !strcmp (p, "YES")
                       || !strcmp (p, "Yes") || !strcmp (p, "1")
                       || !strcmp (p, "true") || !strcmp (p, "TRUE")
                       || !strcmp (p, "True"))
                showScore = 1;

              else
                {
                  printf ("Invalid option for `showScore`\n");
                  goto end;
                }
            }

          else if (!strcmp (s, "--wasd"))
            {
              keyLeft = 'A';
              keyRight = 'D';
              keyDown = 'S';
            }
        }

      if (helpFlag)
        {
          printf ("Usage: %s [OPTIONS]\n", argv[0]);
          printf ("Options:\n");
          printf ("  -h, --help\t\t\t\tShow this help message and exit\n");
          printf ("  --width\t\t\t\tSet the width of the game screen\n");
          printf ("  --height\t\t\t\tSet the height of the game screen\n");
          printf (
              "  -kl, --key-left\t\t\tSet the key to move the shape left\n");
          printf (
              "  -kr, --key-right\t\t\tSet the key to move the shape right\n");
          printf (
              "  -kd, --key-down\t\t\tSet the key to move the shape down\n");
          printf ("  -ks, --key-shift\t\t\tSet the key to shift the shape\n");
          printf ("  -ss, --show-score\t\t\tShow the score\n");
          printf ("  --wasd\t\t\t\tUse standard WASD (ASD) layout\n");
        }
      else
        goto L1;
    }

end:
  return 0;
}

// arrow keys to move, <esc> to quit
DWORD WINAPI
keycatch (LPVOID args)
{
  int c;

  HANDLE stdIn = GetStdHandle (STD_INPUT_HANDLE);
  DWORD ev = 0;
  DWORD ev_read = 0;

  while (!GAMEOVER)
    {
      GetNumberOfConsoleInputEvents (stdIn, &ev);

      if (ev != 0)
        {
          INPUT_RECORD ev_buf[ev];
          ReadConsoleInput (stdIn, ev_buf, ev, &ev_read);

          for (DWORD i = 0; i < ev_read; i++)
            {
              if (ev_buf[i].EventType == KEY_EVENT
                  && ev_buf[i].Event.KeyEvent.bKeyDown)
                {
                  if (curr_falling_shape != NULL
                      && curr_falling_shape->is_falling)
                    ;
                  else
                    for (int i = shape_count - 1; i >= 0; i--)
                      {
                        if (SHAPES[i].is_falling)
                          curr_falling_shape = &SHAPES[i];
                      }

                  if (ev_buf[i].Event.KeyEvent.wVirtualKeyCode == keyLeft)
                    {
                      if (curr_falling_shape->pos_c > 0
                          && shape_boundary_check (curr_falling_shape,
                                                   keyLeft))
                        {
                          curr_falling_shape->pos_c--;
                        }
                    }
                  else if (ev_buf[i].Event.KeyEvent.wVirtualKeyCode
                           == keyRight)
                    {
                      if (curr_falling_shape->pos_c < COLUMN - 1
                          && shape_boundary_check (curr_falling_shape,
                                                   keyRight))
                        {
                          curr_falling_shape->pos_c++;
                        }
                    }
                  else if (ev_buf[i].Event.KeyEvent.wVirtualKeyCode == keyDown)
                    {
                      if (curr_falling_shape->pos_r < ROW - 1
                          && shape_boundary_check (curr_falling_shape,
                                                   keyDown))
                        {
                          curr_falling_shape->pos_r++;
                        }
                    }
                  else if (ev_buf[i].Event.KeyEvent.wVirtualKeyCode
                           == VK_ESCAPE)
                    {
                      GAMEOVER = 1;
                    }
                  else if (ev_buf[i].Event.KeyEvent.wVirtualKeyCode
                           == keyShift)
                    {
                      if (shape_boundary_check (curr_falling_shape, keyShift))
                        {
                          switch (curr_falling_shape->type)
                            {
                            case SHAPE_VLINE:
                              curr_falling_shape->type = SHAPE_HLINE;
                              break;
                            case SHAPE_HLINE:
                              curr_falling_shape->type = SHAPE_VLINE;
                              break;
                            case SHAPE_L_WITHLONGHLINE:
                              curr_falling_shape->type = SHAPE_L_WITHLONGVLINE;
                              break;
                            case SHAPE_L_WITHLONGVLINE:
                              curr_falling_shape->type = SHAPE_L_WITHLONGHLINE;
                              break;
                            case SHAPE_T_VERTICAL:
                              curr_falling_shape->type
                                  = SHAPE_T_HORIZONTAL_MIRROR;
                              break;
                            case SHAPE_T_HORIZONTAL_MIRROR:
                              curr_falling_shape->type = SHAPE_T_MIRROR;
                              break;
                            case SHAPE_T_MIRROR:
                              curr_falling_shape->type = SHAPE_T_HORIZONTAL;
                              break;
                            case SHAPE_T_HORIZONTAL:
                              curr_falling_shape->type = SHAPE_T_VERTICAL;
                              break;
                            default:
                              break;
                            }
                        }
                    }
                }
            }
        }
    }

  return 0;
}

void
add_shape (shape s)
{
  SHAPES[shape_count++] = s;
}

int
bounded_shapeidx (int r, int c)
{
  for (size_t i = 0; i < shape_count; i++)
    {
      shape s = SHAPES[i];

      switch (s.type)
        {
        case SHAPE_HLINE:
          {
            if (r == s.pos_r && (c >= s.pos_c && c < s.pos_c + 3))
              {
                return i;
              }
          }
          break;

        case SHAPE_VLINE:
          {
            if (c == s.pos_c && r >= s.pos_r && r < s.pos_r + 3)
              {
                return i;
              }
          }
          break;

        case SHAPE_L_WITHLONGHLINE:
          {
            if ((r == s.pos_r && c == s.pos_c)
                || (r == s.pos_r + 1 && (c <= s.pos_c + 2 && c >= s.pos_c)))
              {
                return i;
              }
          }
          break;

        case SHAPE_L_WITHLONGVLINE:
          {
            if (((r == s.pos_r + 2) && (c == s.pos_c + 1))
                || ((r >= s.pos_r) && (r < s.pos_r + 3) && (c == s.pos_c)))
              {
                return i;
              }
          }
          break;

        case SHAPE_T_VERTICAL:
          {
            if ((r == s.pos_r && c >= s.pos_c && c <= s.pos_c + 2)
                || (r == s.pos_r + 1 && c == s.pos_c + 1)
                || (r == s.pos_r + 2 && c == s.pos_c + 1))
              {
                return i;
              }
          }
          break;

        case SHAPE_T_HORIZONTAL:
          {
            if ((c == s.pos_c && r >= s.pos_r && r <= s.pos_r + 2)
                || (r == s.pos_r + 1 && c >= s.pos_c && c <= s.pos_c + 2))
              {
                return i;
              }
          }
          break;

        case SHAPE_T_MIRROR:
          {
            if ((c == s.pos_c && r >= s.pos_r && r <= s.pos_r + 2)
                || (r == s.pos_r + 2 && c >= s.pos_c - 1 && c <= s.pos_c + 1))
              {
                return i;
              }
          }
          break;

        case SHAPE_T_HORIZONTAL_MIRROR:
          {
            if ((r == s.pos_r && c >= s.pos_c && c <= s.pos_c + 2)
                || (c == s.pos_c + 2 && r >= s.pos_r - 1 && r <= s.pos_r + 1))
              {
                return i;
              }
          }
          break;

        default:
          break;
        }
    }

  return -1; /* no shape bounded */
}

void
drop_shape (void)
{
  int saw_falling_shape_idx = -1;

  for (size_t i = 0; i < shape_count; i++)
    {
      shape *s = &SHAPES[i];

      if (shape_boundary_check (s, keyDown))
        {
          s->pos_r++;
          saw_falling_shape_idx = 1;
        }
      else
        s->is_falling = 0;
    }

  if (saw_falling_shape_idx == -1)
    {
      // All shapes have landed, make another shape
      add_shape ((shape){
          .is_falling = 1,
          .pos_c = rand_range (0, COLUMN - 1),
          .pos_r = 1,
          .type = rand_range (0, TOTAL_SHAPES),
      });

      shape *l = &SHAPES[shape_count - 1];

      switch (l->type)
        {
        case SHAPE_HLINE:
        case SHAPE_VLINE:
          {
            SCORE += 3;
          }
          break;

        case SHAPE_L_WITHLONGHLINE:
        case SHAPE_L_WITHLONGVLINE:
          {
            SCORE += 4;
          }
          break;

        case SHAPE_T_HORIZONTAL:
        case SHAPE_T_VERTICAL:
        case SHAPE_T_MIRROR:
        case SHAPE_T_HORIZONTAL_MIRROR:
          {
            SCORE += 5;
          }
          break;

        default:
          break;
        }
    }
}

int
shape_boundary_check (shape *s, int dir)
{
  switch (s->type)
    {
    case SHAPE_HLINE:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 3) == -1
                    && s->pos_c + 3 < COLUMN);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 1, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 2) == -1
                    && s->pos_r < ROW - 1);
          }
        else if (dir == keyShift)
          {
            return (bounded_shapeidx (s->pos_r + 1, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c) == -1);
          }
      }
      break;

    case SHAPE_VLINE:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c - 1) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 1) == -1
                    && s->pos_c < COLUMN - 1);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 3, s->pos_c) == -1
                    && s->pos_r + 3 < ROW);
          }
        else if (dir == keyShift)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r, s->pos_c + 2) == -1);
          }
      }
      break;

    case SHAPE_L_WITHLONGHLINE:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c - 1) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 3) == -1
                    && s->pos_c + 3 < COLUMN);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 2, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 2) == -1
                    && s->pos_r + 2 < ROW);
          }
        else if (dir == keyShift)
          {
            return bounded_shapeidx (s->pos_r - 1, s->pos_c) == -1;
          }
      }
      break;

    case SHAPE_L_WITHLONGVLINE:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c - 1) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 2) == -1
                    && s->pos_c + 2 < COLUMN);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 3, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 3, s->pos_c + 1) == -1
                    && s->pos_r + 3 < ROW);
          }
        else if (dir == keyShift)
          {
            return (bounded_shapeidx (s->pos_r + 1, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 2) == -1
                    && s->pos_r + 3 < COLUMN);
          }
      }
      break;

    case SHAPE_T_HORIZONTAL:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c - 1) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 3) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 1) == -1
                    && s->pos_c + 3 < COLUMN);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 3, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 2) == -1
                    && s->pos_r + 3 < ROW);
          }
        else if (dir == keyShift)
          {
            return (1);
          }
      }
      break;

    case SHAPE_T_VERTICAL:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c - 1) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 3) == -1
                    /* && bounded_shapeidx (s->pos_r + 1, s->pos_c) == -1 */
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 2) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 2) == -1
                    && s->pos_c + 3 < COLUMN);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 3, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 2) == -1
                    && s->pos_r + 3 < ROW);
          }
        else if (dir == keyShift)
          {
            return (1);
          }
      }
      break;

    case SHAPE_T_MIRROR:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c - 2) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 1) == -1
                    /* && bounded_shapeidx (s->pos_r + 1, s->pos_c) == -1 */
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 2) == -1
                    && s->pos_c + 3 < COLUMN);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 3, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 3, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r + 3, s->pos_c + 1) == -1
                    && s->pos_r + 3 < ROW);
          }
        else if (dir == keyShift)
          {
            return (1);
          }
      }
      break;

    case SHAPE_T_HORIZONTAL_MIRROR:
      {
        if (dir == keyLeft)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c - 1) == -1
                    && bounded_shapeidx (s->pos_r - 1, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 1) == -1
                    && s->pos_c > 0);
          }
        else if (dir == keyRight)
          {
            return (bounded_shapeidx (s->pos_r, s->pos_c + 3) == -1
                    && bounded_shapeidx (s->pos_r - 1, s->pos_c + 3) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 3) == -1
                    && s->pos_c + 3 < COLUMN);
          }
        else if (dir == keyDown)
          {
            return (bounded_shapeidx (s->pos_r + 1, s->pos_c) == -1
                    && bounded_shapeidx (s->pos_r + 1, s->pos_c + 1) == -1
                    && bounded_shapeidx (s->pos_r + 2, s->pos_c + 2) == -1
                    && s->pos_r + 2 < ROW);
          }
        else if (dir == keyShift)
          {
            return (1);
          }
      }
      break;

    default:
      break;
    }
}

void
clrscr (void)
{
  system ("cls");
}

void
print_screen (void)
{
  char topbar[(COLUMN * 2) + 1];
  memset (topbar, '_', (COLUMN * 2));
  topbar[(COLUMN * 2)] = '\0';

  printf (" %s\n", topbar);

  for (size_t i = 0; i < ROW; i++)
    {
      for (size_t j = 0; j < COLUMN; j++)
        {
          int k = bounded_shapeidx (i, j);

          if (k != -1)
            SCREEN[i][j] = 1;
          else
            SCREEN[i][j] = 0;
        }
    }

  char LINE[ROW][(COLUMN + 2) * 2 + 1];
  size_t lc = 0;

  for (size_t i = 0; i < ROW; i++)
    {
      int saw_0 = 0;

      for (size_t j = 0; j < COLUMN; j++)
        {
          if (!SCREEN[i][j])
            {
              saw_0 = 1;
              break;
            }
        }

      if (!saw_0) // Entire line is filled
        {
          size_t j = i;
          while (j > 0)
            {
              for (size_t k = 0; k < COLUMN; k++)
                {
                  SCREEN[j][k] = SCREEN[j - 1][k];
                }

              j--;
            }

          for (size_t k = 0; k < COLUMN; k++)
            {
              SCREEN[0][k] = 0;
            }
        }
    }

  for (size_t i = 0; i < ROW; i++)
    {
      strcpy (LINE[i], "| ");
      for (size_t j = 0; j < COLUMN; j++)
        {
          if (SCREEN[i][j])
            strcat (LINE[i], "o ");
          else
            strcat (LINE[i], "  ");
        }
      strcat (LINE[i], "|\n");
    }

  for (size_t i = 0; i < ROW; i++)
    {
      printf (LINE[i]);
    }

  if (showScore)
    printf (" %s\nSCORE: %d\n", topbar, SCORE);
  else
    printf (" %s", topbar);

  for (size_t i = 0; i < shape_count; i++)
    {
      if (SHAPES[i].pos_r == 1 && !SHAPES[i].is_falling)
        {
          GAMEOVER = 1;
          return;
        }
    }
}