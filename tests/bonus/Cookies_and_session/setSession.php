<?php

session_start();

for ($i = 0; $i <= 100; $i++)
{
    $_SESSION['name' . $i] = "val" . $i;
}