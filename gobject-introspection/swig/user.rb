require_relative "user.so"

user = User.user_new("Alice", 29)
p User.user_get_name(user) # -> "Alice"
p User.user_get_age(user)  # -> 29
User.user_free(user)
