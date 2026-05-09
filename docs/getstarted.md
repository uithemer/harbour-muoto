---
layout: default
title: Get started
nav_order: 2
permalink: docs/getstarted
has_children: true
has_toc: true
---

# Get started

Informations on how create theme packs compatible with my script. Use my [dummy package](https://github.com/uithemer/harbour-themepack-dummy) as reference, it includes the basic directory tree and an example spec file.

## Requisites

* A basic knowledge on how to use the Linux terminal and compile rpm's.

## Icons, fonts, sounds

Read the page relative to the theme part you're creating, then come back here.

* [Icons](icons)
* [Fonts](fonts)
* [Sounds](sounds)

## .spec file

Open the .spec file and edit these lines:
* **Name**: the name used in the system to identify your package.
* **Version**: the version of the package.
* **Release**: the issue of the version.
* **Summary**: an intellegible name for your package.
* **Vendor**: your name/nickname.
* **Packager**: your name/nickname and email (optional).
* **URL**: your blog/website (optional).
* **description**: description for your package.
* **changelog**: the changelog of your package.

### Notes

* The name of the package must start with *harbour-themepack-* e.g. *harbour-themepack-mypackage*.
* The file *package* contains the intelligible name of your icon pack e.g. *My theme pack*. Keep it on one line.

## Building

[This guide](http://talk.maemo.org/showthread.php?t=92963) was useful to me to get started.
As a *theme pack* is a bunch of icons and fonts, it makes sense to make it architecture agnostic. To do so, append modify the rpmbuild syntax as follows:

`rpmbuild --bb --target noarch PATHOFTHESPECFILE/harbour-themepack-mypackage.spec`

## Themepack helper

To resize svg icons you can also use my [themepack-helper](https://github.com/uithemer/themepack-helper).

## Companion app

If you are familiar with the Sailfish SDK you can use my companion app model. It includes a script which automates the listing of missing icons, creating an e-mail draft with a predefined address of your choice, for minimal effort from the user.

Take a look at it [here](https://github.com/uithemer/harbour-themepack-companion). Feel free to fork it and use it on your projects.

## Releasing

You are ready to build the package and publish it! The preferred method is on 
[openrepos.net](https://openrepos.net). Don't forget to add as a notice to enable [my 
repository](https://openrepos.net/user/583/repository) and have at least Theme pack support 0.0.5 
installed, otherwise it won't work!
