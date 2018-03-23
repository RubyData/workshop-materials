#include <stdlib.h>
#include <string.h>

#include "user.h"

struct User {
  char *name;
  int age;
};

User *
user_new(const char *name, int age)
{
  User *user = malloc(sizeof(User));
  user->name = strdup(name);
  user->age = age;
  return user;
}

void
user_free(User *user)
{
  free(user);
}

const char *
user_get_name(User *user)
{
  return user->name;
}

int
user_get_age(User *user)
{
  return user->age;
}
