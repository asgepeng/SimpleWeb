#include "layoutmanager.h"

std::unordered_map<std::string, std::string> LayoutManager::layouts;
void LayoutManager::Load()
{
	layouts["main"] = R"(<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <title>Products</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Montserrat+Underline:ital,wght@0,100..900;1,100..900&family=Montserrat:ital,wght@0,100..900;1,100..900&family=Open+Sans:ital,wght@0,300..800;1,300..800&family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="/assets/main.min.css"/>
</head>
<body>
    <nav>
        <ul>
            <li>
                File
                <ul>
                    <li><a href="/home/logout">Logout</a></li>
                </ul>
            </li>
            <li>
                Data
                <ul>
                    <li><a href="/products">Products</a></li>
                    <li><a href="/suppliers">Suppliers</a></li>
                    <li><a href="/customers">Customers</a></li>
                    <li><a href="/ap">Account Payable</a></li>
                    <li><a href="/ar">Account Receivable</a></li>
                </ul>
            </li>
            <li>
                Transaksi
                <ul>
                    <li><a href="/stock-opname">Stock Opname</a></li>
                    <li><a href="/purchases">Pembelian</a></li>
                    <li><a href="/sales">Penjualan</a></li>
                </ul>
            </li>
            <li>
                Laporan
                <ul>
                    <li><a href="/report/stock-opname">Laporan Stok Opname</a></li>
                    <li><a href="/reports/purchases">Laporan Pembelian</a></li>
                    <li><a href="/reports/sales">Laporan Penjualan</a></li>
                </ul>
            </li>
        </ul>
        <input type="text" class="search" id="search" placeholder="Search"/>
    </nav>
    <main style="padding:0;">)";
}
