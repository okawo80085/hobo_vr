from setuptools import setup, find_packages
from virtualreality import __version__

with open("pypi_readme.md", "r") as f:
    long_description = f.read()

setup(
    name="virtualreality",
    version=__version__,
    author="Okawo <okawo.198@gmail.com>, SimLeek <simulator.leek@gmail.com>",
    author_email="",
    description="python side of hobo_vr",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/okawo80085/hobo_vr",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 2 - Pre-Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Education",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: Healthcare Industry",
        "Intended Audience :: Information Technology",
        "Intended Audience :: Manufacturing",
        "Intended Audience :: Science/Research",
        "Intended Audience :: Telecommunications Industry",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3.7",
        "Topic :: Games/Entertainment",
        "Topic :: Multimedia :: Graphics :: Viewers",
        "Topic :: Scientific/Engineering :: Human Machine Interfaces",
        "Topic :: System :: Hardware :: Hardware Drivers",
    ],
    install_requires=[
        "numpy",
        "pykalman",
        "pyserial",
        "docopt",
        "aioconsole",
        "pyrr",
        "scipy",
    ],
    extras_require={"camera": ["displayarray", "opencv-python"], "bluetooth": ["pybluez"]},
    entry_points={"console_scripts": ["pyvr=virtualreality.main:main"]},
    python_requires=">=3.7",
)
