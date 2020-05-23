from setuptools import setup, find_packages

with open("README.md", "r") as f:
    long_description = f.read()

setup(
    name="virtualreality",
    version="0.1",
    author="",
    author_email="",
    description="",
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
        "Programming Language :: Python :: 3 :: Only",
        "Topic :: Games/Entertainment",
        "Topic :: Multimedia :: Graphics :: Viewers",
        "Topic :: Scientific/Engineering :: Human Machine Interfaces",
        "Topic :: System :: Hardware :: Hardware Drivers",
    ],
    install_requires=["keyboard", "opencv-python", "numpy", "pykalman", "pyserial", "docopt", "pyrr"],
    extras_require={"image": ["imutils"]},
    entry_points={"console_scripts": ["pyvr=virtualreality.main:main"]},
)
