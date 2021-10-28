<?php

for ($i = 1; $i <= 100; $i++) {
    echo $_COOKIE["name" . $i] . "<br>";
}

// print_r($_COOKIE);