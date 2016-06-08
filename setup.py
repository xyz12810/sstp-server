#!/usr/bin/env python2
from setuptools import setup, Extension

with open('README.rst') as readme:
    long_description = readme.read()

fcsmodule = Extension('sstpd.codec', sources=['sstpd/codecmodule.c'])

setup(
    name='sstp-server',
    version='0.3.5',
    description='Secure Socket Tunneling Protocol (SSTP) VPN server.',
    author='Sorz',
    author_email='orz@sorz.org',
    url='https://github.com/sorz/sstp-server',
    packages=['sstpd'],
    ext_modules = [fcsmodule],
    data_files=[('', ['README.rst'])],
    entry_points="""
    [console_scripts]
    sstpd = sstpd:main
    """,
    install_requires=[
        'twisted', 'service_identity', 'argparse', 'py2-ipaddress'
    ],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Console',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 2 :: Only',
        'Topic :: Internet',
        'Topic :: Internet :: Proxy Servers',
        'License :: OSI Approved :: MIT License'
    ],
    long_description=long_description
)

