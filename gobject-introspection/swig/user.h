#pragma once

typedef struct User User;

User *user_new(const char *name, int age);
void user_free(User *user);

const char *user_get_name(User *user);
int user_get_age(User *user);
