let currentPage = 1;
const filesPerPage = 5;
let allFiles = [];

// 获取当前页面 URL 并设置高亮
const links = document.querySelectorAll('.sidebar a');
requestAnimationFrame(() => {
    links.forEach(link => {
        if (link.href === window.location.href) {
            link.classList.add('active');
        }
    });
});

// 异步更新进度（暂时没启用）
async function updateProgress() {
    try {
        const response = await fetch(`/progress.txt`);
        if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);

        const progress = await response.text();
        const progressValue = parseInt(progress.trim());

        if (!isNaN(progressValue)) {
            document.getElementById("uploadProgress").value = progressValue;
            document.getElementById("progressText").innerText = `上传进度: ${progressValue}%`;

            if (progressValue < 100) {
                setTimeout(updateProgress, 800);
            }
        } else {
            console.error("进度数据解析失败:", progress);
        }
    } catch (error) {
        console.error("获取进度时出错:", error);
    }
}

// 提交上传表单
document.getElementById("uploadForm").onsubmit = async function (event) {
    event.preventDefault();

    let fileInput = document.getElementById("fileInput");
    if (!fileInput.files.length) {
        alert("请选择文件!");
        return;
    }

    let formData = new FormData();
    formData.append("file", fileInput.files[0]);

    let message = document.getElementById("message");
    message.innerText = "上传中...";
    message.style.color = "orange";

    try {
        let response = await fetch("/upload", {
            method: "POST",
            body: formData
        });

        let text = await response.text();
        console.log("服务器返回:", response.status, text);

        if (response.ok) {
            message.innerText = "上传成功!";
            message.style.color = "green";
            refreshFileList(); // 上传成功后刷新文件列表
        } else {
            message.innerText = "上传失败!";
            message.style.color = "red";
        }
    } catch (err) {
        console.error("上传出错:", err);
        message.innerText = "上传异常!";
        message.style.color = "red";
    }
};

// 获取文件列表并填充到页面
function refreshFileList() {
    fetch(`/filelist.json?page=${currentPage}`) //传递分页参数
        .then(response => response.json())
        .then(files => {
            const list = document.getElementById("file-list");
            list.innerHTML = "";

            if (files.length === 0) {
                list.innerHTML = "<li>暂无文件</li>";
                return;
            }

            files.forEach(file => {
                const li = document.createElement("li");

                const nameLink = document.createElement("a");
                nameLink.href = "/" + file.name;
                nameLink.textContent = file.name;
                nameLink.target = "_blank";

                const sizeSpan = document.createElement("span");
                sizeSpan.textContent = ` 大小: ${(file.size / 1024).toFixed(1)} KB`;

                const timeSpan = document.createElement("span");
                timeSpan.textContent = ` 修改时间: ${file.modified}`;

                const downloadBtn = document.createElement("button");
                downloadBtn.textContent = "下载";
                downloadBtn.onclick = () => {
                    window.location.href = `/download?file=${encodeURIComponent(file.name)}`;
                };

                const deleteBtn = document.createElement("button");
                deleteBtn.textContent = "删除";
                deleteBtn.style.marginLeft = "10px";
                deleteBtn.onclick = () => {
                    deleteFile(file.name);
                };

                li.appendChild(nameLink);
                li.appendChild(sizeSpan);
                li.appendChild(timeSpan);
                li.appendChild(downloadBtn);
                li.appendChild(deleteBtn);

                list.appendChild(li);
            });

            // 更新按钮状态
            document.getElementById("page-info").innerText = `第 ${currentPage} 页`;
            updatePaginationButtons(files.length);
        })
        .catch(err => {
            console.error("文件列表加载失败:", err);
        });
}

function updatePaginationButtons(receivedFileCount) {
    document.getElementById("prev-page").disabled = currentPage === 1;
    document.getElementById("next-page").disabled = receivedFileCount < filesPerPage;
}

document.getElementById("prev-page").addEventListener("click", () => {
    if (currentPage > 1) {
        currentPage--;
        refreshFileList();
    }
});

document.getElementById("next-page").addEventListener("click", () => {
    currentPage++;
    refreshFileList();
});


// 删除文件函数
function deleteFile(filename) {
    if (!confirm(`确定要删除 ${filename} 吗？`)) return;

    fetch(`/delete?file=${encodeURIComponent(filename)}`, {
        method: "GET"
    })
        .then(res => {
            if (res.ok) {
                refreshFileList();
            } else {
                alert("删除失败！");
            }
        });
}


// 页面加载完成后立即拉取文件列表
window.addEventListener("DOMContentLoaded", refreshFileList);
