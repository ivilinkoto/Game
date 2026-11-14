package online.hotfix.client

import android.app.Activity
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.pm.ApplicationInfo
import android.content.pm.PackageInfo
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.Typeface
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.view.Gravity
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import java.io.File
import java.util.zip.ZipEntry
import java.util.zip.ZipFile

/**
 * Етап 0: диагностика.
 *
 * Приложението НЕ съдържа файлове на играта. То проверява инсталираната на
 * устройството GTA: San Andreas (от Google Play) и събира данните, нужни, за да
 * решим как клиентът ще я зарежда: версия, архитектура, къде са библиотеките,
 * дали са достъпни данните на играта и т.н.
 */
class MainActivity : Activity() {

    companion object {
        const val GAME_PACKAGE = "com.rockstargames.gtasa"
        private const val BRAND = 0xFFFF6A00.toInt()
    }

    private lateinit var reportView: TextView
    private var report: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(buildUi())
        runDiagnostics()
    }

    // ------------------------------------------------------------------
    //  Интерфейс (без XML и без библиотеки – само Android)
    // ------------------------------------------------------------------

    private fun buildUi(): ScrollView {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(24), dp(24), dp(24), dp(24))
            setBackgroundColor(Color.parseColor("#14161C"))
        }

        root.addView(TextView(this).apply {
            text = "HOTFIX ONLINE"
            textSize = 28f
            setTextColor(BRAND)
            typeface = Typeface.DEFAULT_BOLD
        })
        root.addView(TextView(this).apply {
            text = "Клиент – етап 0: проверка на устройството"
            textSize = 15f
            setTextColor(Color.parseColor("#A0A0A0"))
            setPadding(0, 0, 0, dp(16))
        })

        val buttons = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.START
        }
        buttons.addView(makeButton("Копирай доклада") { copyReport() })
        buttons.addView(makeButton("Сподели") { shareReport() })
        buttons.addView(makeButton("Обнови") { runDiagnostics() })
        buttons.addView(makeButton("Пусни GTA SA (тест)") { launchGame() })
        root.addView(buttons)

        reportView = TextView(this).apply {
            textSize = 13f
            setTextColor(Color.WHITE)
            typeface = Typeface.MONOSPACE
            setTextIsSelectable(true)
            setPadding(0, dp(16), 0, 0)
            text = "Проверявам…"
        }
        root.addView(reportView, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT))

        return ScrollView(this).apply {
            setBackgroundColor(Color.parseColor("#14161C"))
            addView(root, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT))
        }
    }

    private fun makeButton(label: String, onClick: () -> Unit): Button =
        Button(this).apply {
            text = label
            isAllCaps = false
            setTextColor(Color.WHITE)
            setBackgroundColor(BRAND)
            setOnClickListener { onClick() }
            layoutParams = LinearLayout.LayoutParams(WRAP_CONTENT, WRAP_CONTENT).apply {
                marginEnd = dp(8)
            }
        }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    // ------------------------------------------------------------------
    //  Действия
    // ------------------------------------------------------------------

    private fun copyReport() {
        val clipboard = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        clipboard.setPrimaryClip(ClipData.newPlainText("HOTFIX диагностика", report))
        Toast.makeText(this, "Докладът е копиран", Toast.LENGTH_SHORT).show()
    }

    private fun shareReport() {
        val intent = Intent(Intent.ACTION_SEND).apply {
            type = "text/plain"
            putExtra(Intent.EXTRA_TEXT, report)
        }
        startActivity(Intent.createChooser(intent, "Сподели доклада"))
    }

    private fun launchGame() {
        val intent = packageManager.getLaunchIntentForPackage(GAME_PACKAGE)
        if (intent == null) {
            // Няма я – водим към официалната страница в Google Play
            startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=$GAME_PACKAGE")))
            return
        }
        startActivity(intent)
    }

    // ------------------------------------------------------------------
    //  Диагностика
    // ------------------------------------------------------------------

    private fun runDiagnostics() {
        reportView.text = "Проверявам…"
        Thread {
            val text = try {
                buildReport()
            } catch (t: Throwable) {
                "Грешка при проверката: $t\n" + t.stackTraceToString()
            }
            runOnUiThread {
                report = text
                reportView.text = text
            }
        }.start()
    }

    private fun buildReport(): String {
        val sb = StringBuilder()
        val own = packageManager.getPackageInfo(packageName, 0)

        sb.appendLine("=== HOTFIX ONLINE диагностика ${own.versionName} ===")
        sb.appendLine()
        sb.appendLine("[Устройство]")
        sb.appendLine("Модел: ${Build.MANUFACTURER} ${Build.MODEL}")
        sb.appendLine("Android: ${Build.VERSION.RELEASE} (API ${Build.VERSION.SDK_INT})")
        sb.appendLine("ABI: ${Build.SUPPORTED_ABIS.joinToString()}")
        sb.appendLine()

        val game: PackageInfo? = try {
            packageManager.getPackageInfo(GAME_PACKAGE, 0)
        } catch (e: PackageManager.NameNotFoundException) {
            null
        }

        sb.appendLine("[GTA: San Andreas]")
        if (game == null) {
            sb.appendLine("НЕ Е ИНСТАЛИРАНА ($GAME_PACKAGE)")
            sb.appendLine("Инсталирай я от Google Play и натисни \"Обнови\".")
            return sb.toString()
        }
        val ai: ApplicationInfo = game.applicationInfo ?: run {
            sb.appendLine("Няма applicationInfo")
            return sb.toString()
        }

        val versionCode = if (Build.VERSION.SDK_INT >= 28) game.longVersionCode else @Suppress("DEPRECATION") game.versionCode.toLong()
        sb.appendLine("Версия: ${game.versionName} (код $versionCode)")
        sb.appendLine("Инсталирана от: ${installerOf(GAME_PACKAGE)}")
        sb.appendLine("targetSdk: ${ai.targetSdkVersion}")
        sb.appendLine("extractNativeLibs: ${(ai.flags and ApplicationInfo.FLAG_EXTRACT_NATIVE_LIBS) != 0}")
        sb.appendLine()

        sb.appendLine("[APK файлове]")
        val apks = mutableListOf(ai.sourceDir)
        ai.splitSourceDirs?.let { apks.addAll(it) }
        for (path in apks) {
            val f = File(path)
            sb.appendLine("${f.name}: ${mb(f.length())}, четим: ${f.canRead()}")
        }
        sb.appendLine()

        sb.appendLine("[Native библиотеки в APK-тата]")
        for (path in apks) {
            try {
                ZipFile(path).use { zip ->
                    var assets = 0
                    var assetsBytes = 0L
                    for (e in zip.entries()) {
                        if (e.name.startsWith("lib/") && e.name.endsWith(".so")) {
                            val method = if (e.method == ZipEntry.STORED) "STORED" else "DEFLATED"
                            sb.appendLine("${File(path).name} -> ${e.name} (${mb(e.size)}, $method)")
                        } else if (e.name.startsWith("assets/")) {
                            assets++
                            assetsBytes += e.size
                        }
                    }
                    if (assets > 0) sb.appendLine("${File(path).name} -> assets/: $assets файла, ${mb(assetsBytes)}")
                }
            } catch (t: Throwable) {
                sb.appendLine("${File(path).name}: не може да се отвори ($t)")
            }
        }
        sb.appendLine()

        sb.appendLine("[Разархивирани библиотеки]")
        sb.appendLine("Папка: ${ai.nativeLibraryDir}")
        val libDir = File(ai.nativeLibraryDir ?: "")
        val libs = libDir.listFiles()
        if (libs == null) {
            sb.appendLine("Няма достъп или е празна")
        } else if (libs.isEmpty()) {
            sb.appendLine("Празна (библиотеките са вътре в APK)")
        } else {
            for (lib in libs.sortedBy { it.name }) {
                sb.appendLine("${lib.name}: ${mb(lib.length())}, четим: ${lib.canRead()}")
            }
        }
        sb.appendLine()

        sb.appendLine("[Данни на играта]")
        val ext = Environment.getExternalStorageDirectory()
        describeDir(sb, "OBB", File(ext, "Android/obb/$GAME_PACKAGE"))
        describeDir(sb, "Data", File(ext, "Android/data/$GAME_PACKAGE"))
        describeDir(sb, "Data/files", File(ext, "Android/data/$GAME_PACKAGE/files"))

        return sb.toString()
    }

    private fun describeDir(sb: StringBuilder, label: String, dir: File) {
        val files = dir.listFiles()
        sb.append("$label (${dir.path}): съществува=${dir.exists()}, четима=${dir.canRead()}")
        if (files == null) {
            sb.appendLine(", списък: няма достъп")
            return
        }
        sb.appendLine(", ${files.size} елемента")
        for (f in files.sortedBy { it.name }.take(15)) {
            sb.appendLine("   ${f.name}${if (f.isDirectory) "/" else ""} ${if (f.isFile) mb(f.length()) else ""}")
        }
    }

    private fun installerOf(pkg: String): String = try {
        if (Build.VERSION.SDK_INT >= 30) {
            packageManager.getInstallSourceInfo(pkg).installingPackageName ?: "неизвестно"
        } else {
            @Suppress("DEPRECATION")
            packageManager.getInstallerPackageName(pkg) ?: "неизвестно"
        }
    } catch (t: Throwable) {
        "грешка: $t"
    }

    private fun mb(bytes: Long): String = String.format("%.1f MB", bytes / 1048576.0)
}
