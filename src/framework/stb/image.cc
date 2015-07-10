
#define STB_IMAGE_IMPLEMENTATION

// We only want png and jpg support.
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG

#include <stb/stb_image.h>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>



#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>
