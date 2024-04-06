
from PIL import Image
import numpy as np
import io
import matplotlib.pyplot as plt

# for caching images
import hashlib
from pathlib import Path
import os
from multiprocessing import Process

import gzip


def store_image(filepath, bytes_io_value):
    with open(filepath, 'wb') as out:
        out.write(bytes_io_value)


def array_to_image(array, image_type, app_data_path):

    cached_dir = os.path.join(app_data_path, 'cached_images')
    max_cached_size_mb = 100

    # move value on uint8 full scale and
    # convert it to uint8 type
    if np.amax(array) != 0:
        array = array.astype(np.float32)
        array -= array.min()

        max_value = np.iinfo('u1').max
        array *= float(max_value)/array.max()

    array = array.astype('u1')

    h = hashlib.sha1(array)
    filename = h.hexdigest()

    if image_type == "PNG":
        filename += '.png'
    elif image_type == "JPEG":
        filename += '.jpg'
    else:
        filename += '.png'

    Path(cached_dir).mkdir(exist_ok=True)

    filepath = os.path.join(cached_dir, filename)

    # if file exists, read it and return its bytes
    if Path(filepath).exists():
        with open(filepath, 'rb') as f:
            bytes_io = io.BytesIO(f.read())
            return bytes_io.getvalue()

            # If we want to compress file, uncomment following lines
            # bytes_io_value = bytes_io.getvalue()
            # commpressed_bytes_io = gzip.compress(bytes_io_value)
            # return commpressed_bytes_io, 200, {
            #     'Content-Encoding': 'gzip',
            #     'Content-Length': len(commpressed_bytes_io)
            # }

    # else generate an image, store it and return its bytes
    shape = array.shape
    if len(shape) == 2:
        shape = (shape[0], shape[1], 1)
        array = np.reshape(array, shape, order='F')

    array = np.reshape(array, (shape[0], shape[1]*shape[2]), order='F')

    # Get the color map by name:
    color_map = 'jet'
    # color_map = 'gray'
    cm = plt.get_cmap(color_map)

    # Apply the colormap like a function to any array:
    colored_image = cm(array)
    im = Image.fromarray((colored_image[:, :, :3] * 255).astype(np.uint8))

    bytes_io = io.BytesIO()

    if image_type == "PNG":
        im.save(bytes_io, "PNG")
    elif image_type == "JPEG":
        im.save(bytes_io, "JPEG")
    else:
        im.save(bytes_io, "PNG")

    # get cached dir content size
    total_bytes = sum(
        f.stat().st_size for f in Path(cached_dir).glob('**/*') if f.is_file()
    )

    print('total_bytes: {}'.format(total_bytes))

    # while its size is over maximum, remove files beginning by the newer
    #   until total size is under maximum
    if total_bytes/1024/1024 > max_cached_size_mb:

        paths = sorted(
            Path(cached_dir).iterdir(),
            key=os.path.getmtime,
            reverse=False)

        for path in paths:
            file_size = Path(path).stat().st_size

            os.remove(path)

            total_bytes -= file_size

            if total_bytes/1024/1024 < max_cached_size_mb:
                break

    bytes_io_value = bytes_io.getvalue()

    # start store process
    # proc = Process(target=store_image, args=(filepath, bytes_io_value))

    # or do it in the current thread
    store_image(filepath, bytes_io_value)

    return bytes_io_value
