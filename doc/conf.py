# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-FileCopyrightText: Copyright (c) 2024-2026 Henrik Brix Andersen <henrik@brixandersen.dk>
#
# SPDX-License-Identifier: Apache-2.0

# Configuration file for the Sphinx documentation builder

project = 'zephyr-i2c-tiny-usb-bridge'
copyright = '2026, Muhammad Waleed Badar'
author = 'Muhammad Waleed Badar'

extensions = [
    'sphinx_copybutton',
    'sphinx_rtd_theme',
    'sphinx.ext.githubpages',
    'sphinx.ext.intersphinx',
]

exclude_patterns = ['build', 'Thumbs.db', '.DS_Store']

intersphinx_mapping = {'zephyr': ('https://docs.zephyrproject.org/latest/', None)}

html_baseurl = 'https://walidbadar.github.io/zephyr-i2c-tiny-usb-bridge/'
html_title = 'i2c-tiny-usb USB to I2C bridge firmware'
html_last_updated_fmt = "%Y-%m-%d"
html_show_sourcelink = False
html_show_sphinx = False
html_theme = 'sphinx_rtd_theme'
html_copy_source = False
html_theme_options = {
    'collapse_navigation': False,
    'navigation_depth': 2,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': True,
}
html_context = {
    'display_github': True,
    'github_user': 'walidbadar',
    'github_repo': 'zephyr-i2c-tiny-usb-bridge',
    'github_version': 'main',
    'conf_py_path': '/doc/',
}
