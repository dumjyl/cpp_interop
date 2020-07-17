#define priv private:

#define pub public:

#define fn auto

// disallow copying of the class or struct T
#define NO_COPY(T)                                                                                 \
   pub T(const T& other) = delete;                                                                 \
   pub T& operator=(const T& other) = delete;

// declare a read only field
#define READ_ONLY(T, name)                                                                         \
   priv T _##name;                                                                                 \
   pub fn name() const -> const T& {                                                                 \
      return _##name;                                                                              \
   }

