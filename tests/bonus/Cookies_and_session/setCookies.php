<?php

for ($i = 1; $i <= 100; $i++) {
    setcookie("name" . $i, "val" . $i);
}